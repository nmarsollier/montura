/* Mount — mount_alpaca_bridge.c
 *
 * Bridge functions for the ASCOM Alpaca REST layer.
 *
 * The Alpaca layer (`rest_alpaca/`) only calls functions declared in mount.h.
 * Any access to other modules — motors, network, system time — is bridged
 * through this file so the dependency boundary stays clean.
 *
 * Dependencies:
 *   mount.h / mount_internal.h — settings, MountResult constructors
 *   motors.h                    — MotorsState, motors_current_state()
 *   <math.h>                    — trigonometry for alt/az
 *   <time.h>                    — gmtime(), time() for UTC / LST
 */

#include "mount.h"
#include "mount_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "motors.h"

/* ═══════════════════════════════════════════════════════════════
 * Mutable Alpaca state
 *
 * Holds transient values that Alpaca clients can read and write
 * (target coordinates, pier side, slew settle time, park position).
 * These do not need NVS persistence — they reset on reboot.
 * ═══════════════════════════════════════════════════════════════ */

static struct {
    float target_ra;        /* Target right ascension (hours) */
    float target_dec;       /* Target declination (degrees) */
    int   side_of_pier;     /* 0 = East, 1 = West */
    int   slew_settle_time; /* Settle time after slew (seconds) */
    float park_ra_deg;      /* Park position — RA axis (degrees) */
    float park_dec_deg;     /* Park position — DEC axis (degrees) */
} AlpacaState = {
    .target_ra       = 0.0f,
    .target_dec      = 0.0f,
    .side_of_pier    = 0,
    .slew_settle_time = 0,
    .park_ra_deg     = 0.0f,
    .park_dec_deg    = 90.0f,
};

/* ═══════════════════════════════════════════════════════════════
 * Time helpers
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Convert a Unix timestamp to Julian Date.
 * Used as input to the GMST and alt/az calculations.
 */
static double unix_to_jd(time_t t) {
    return (double) t / 86400.0 + 2440587.5;
}

/*
 * Compute Greenwich Mean Sidereal Time (GMST) in hours from a Julian Date.
 * Uses the standard USNO approximation valid to ~1 arcsecond.
 */
static double gmst_hours(double jd) {
    double T = (jd - 2451545.0) / 36525.0;
    double gmst = 280.46061837 + 360.98564736629 * (jd - 2451545.0)
                  + 0.000387933 * T * T - (T * T * T) / 38710000.0;
    gmst = fmod(gmst, 360.0);
    if (gmst < 0.0) gmst += 360.0;
    return gmst / 15.0;
}

/* ═══════════════════════════════════════════════════════════════
 * Horizontal coordinates (altitude / azimuth)
 *
 * Calculated from the current equatorial position, site location,
 * and UTC time. The results are approximate — no atmospheric
 * refraction correction is applied.
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Current telescope altitude above the horizon (degrees).
 * 90° = zenith, 0° = horizon, negative = below horizon.
 */
float mount_alpaca_get_altitude(void) {
    float ra_h = mount_get_ra_hours();
    float dec_d = mount_get_dec_deg();

    time_t now = time(NULL);
    double jd = unix_to_jd(now);
    double gmst_h = gmst_hours(jd);
    double lst_h = gmst_h + (double) mount_internal_state.lon / 15.0;
    double ha_rad = (lst_h - (double) ra_h) * 15.0 * M_PI / 180.0;
    double dec_rad = (double) dec_d * M_PI / 180.0;
    double lat_rad = (double) mount_internal_state.lat * M_PI / 180.0;

    double sin_alt = sin(dec_rad) * sin(lat_rad)
                     + cos(dec_rad) * cos(lat_rad) * cos(ha_rad);
    return (float) (asin(sin_alt) * 180.0 / M_PI);
}

/*
 * Current telescope azimuth (degrees, 0° = North, 90° = East).
 */
float mount_alpaca_get_azimuth(void) {
    float ra_h = mount_get_ra_hours();
    float dec_d = mount_get_dec_deg();

    time_t now = time(NULL);
    double jd = unix_to_jd(now);
    double gmst_h = gmst_hours(jd);
    double lst_h = gmst_h + (double) mount_internal_state.lon / 15.0;
    double ha_rad = (lst_h - (double) ra_h) * 15.0 * M_PI / 180.0;
    double dec_rad = (double) dec_d * M_PI / 180.0;
    double lat_rad = (double) mount_internal_state.lat * M_PI / 180.0;

    double alt_rad = asin(sin(dec_rad) * sin(lat_rad)
                          + cos(dec_rad) * cos(lat_rad) * cos(ha_rad));
    double cos_az = (sin(dec_rad) - sin(alt_rad) * sin(lat_rad))
                    / (cos(alt_rad) * cos(lat_rad));
    if (cos_az > 1.0) cos_az = 1.0;
    if (cos_az < -1.0) cos_az = -1.0;
    double az_rad = acos(cos_az);
    if (sin(ha_rad) > 0.0) az_rad = 2.0 * M_PI - az_rad;
    return (float) (az_rad * 180.0 / M_PI);
}

/* ═══════════════════════════════════════════════════════════════
 * Sidereal time
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Local Apparent Sidereal Time at the configured site (hours, 0–24).
 * Computed as GMST corrected by site longitude.
 */
float mount_alpaca_get_sidereal_time(void) {
    time_t now = time(NULL);
    double jd = unix_to_jd(now);
    double gmst_h = gmst_hours(jd);
    double lst_h = gmst_h + (double) mount_internal_state.lon / 15.0;
    while (lst_h < 0.0) lst_h += 24.0;
    while (lst_h >= 24.0) lst_h -= 24.0;
    return (float) lst_h;
}

/* ═══════════════════════════════════════════════════════════════
 * UTC date
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Current UTC date as an ISO 8601 string ("2025-06-05T20:15:00").
 * The returned pointer refers to a static buffer — copy it if needed
 * across multiple calls.
 */
const char *mount_alpaca_get_utc_date(void) {
    static char buf[64];
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
             utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday,
             utc->tm_hour, utc->tm_min, utc->tm_sec);
    return buf;
}

/*
 * Set the system clock from an ISO 8601 string.
 * Delegates to mount_set_system_time().
 */
MountResult mount_alpaca_set_utc_date(const char *iso_time) {
    return mount_set_system_time(iso_time);
}

/* ═══════════════════════════════════════════════════════════════
 * Mount state queries
 *
 * Each function reads the authoritative MotorsState from the motors
 * module and returns a boolean matching the Alpaca property.
 * ═══════════════════════════════════════════════════════════════ */

bool mount_alpaca_get_is_slewing(void) {
    MotorsState s = motors_current_state();
    return s.status == MOUNT_STATUS_SLEWING;
}

bool mount_alpaca_get_is_tracking(void) {
    MotorsState s = motors_current_state();
    return s.status == MOUNT_STATUS_TRACKING;
}

bool mount_alpaca_get_is_parked(void) {
    MotorsState s = motors_current_state();
    return s.status == MOUNT_STATUS_PARKED;
}

/*
 * Home is an approximation: the mount is considered "at home" when
 * it is READY and both axes are within 1° of the origin (0, 0).
 */
bool mount_alpaca_get_is_home(void) {
    MotorsState s = motors_current_state();
    return s.status == MOUNT_STATUS_READY &&
           fabsf(s.ra_position) < 1.0f &&
           fabsf(s.dec_position) < 1.0f;
}

/* ═══════════════════════════════════════════════════════════════
 * Site location
 *
 * Read from and written to the persisted mount settings (NVS).
 * The settings struct is copied, modified, and re-persisted on each
 * write to keep NVS consistent.
 * ═══════════════════════════════════════════════════════════════ */

float mount_alpaca_get_site_latitude(void) {
    return mount_internal_state.lat;
}

float mount_alpaca_get_site_longitude(void) {
    return mount_internal_state.lon;
}

int mount_alpaca_get_site_elevation(void) {
    return mount_internal_state.elevation;
}

MountResult mount_alpaca_set_site_latitude(float lat) {
    MountSettings s = mount_internal_state;
    s.lat = lat;
    return mount_update_settings(s);
}

MountResult mount_alpaca_set_site_longitude(float lon) {
    MountSettings s = mount_internal_state;
    s.lon = lon;
    return mount_update_settings(s);
}

MountResult mount_alpaca_set_site_elevation(int elevation) {
    MountSettings s = mount_internal_state;
    s.elevation = elevation;
    return mount_update_settings(s);
}

/* ═══════════════════════════════════════════════════════════════
 * Tracking rate
 *
 * Maps between the mount's internal TrackingMode enum and the
 * ASCOM DriveRates integer values (0 = sidereal, 1 = lunar,
 * 2 = solar, 3 = king — not implemented).
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Return the current tracking mode as an ASCOM DriveRates value.
 */
int mount_alpaca_get_tracking_rate(void) {
    MotorsState s = motors_current_state();
    switch (s.tracking) {
        case TRACKING_SIDEREAL: return 0;
        case TRACKING_LUNAR:    return 1;
        case TRACKING_SOLAR:    return 2;
        default:                return 0;
    }
}

/*
 * True if any automatic tracking mode is active (sidereal, lunar, or solar).
 * Manual and None are considered "not tracking".
 */
bool mount_alpaca_get_tracking(void) {
    MotorsState s = motors_current_state();
    return s.tracking != TRACKING_NONE && s.tracking != TRACKING_MANUAL;
}

/*
 * Enable (= sidereal) or disable (= none) tracking.
 */
MountResult mount_alpaca_set_tracking(bool enabled) {
    if (enabled)
        return mount_set_tracking(TRACKING_SIDEREAL);
    else
        return mount_set_tracking(TRACKING_NONE);
}

/*
 * Set tracking to a specific ASCOM DriveRates value.
 */
MountResult mount_alpaca_set_tracking_rate(int rate) {
    TrackingMode mode;
    switch (rate) {
        case 0: mode = TRACKING_SIDEREAL; break;
        case 1: mode = TRACKING_LUNAR;    break;
        case 2: mode = TRACKING_SOLAR;    break;
        default: mode = TRACKING_SIDEREAL; break;
    }
    return mount_set_tracking(mode);
}

/*
 * Fill `buf` with the ASCOM name for tracking rate `idx`
 * (0 = driveSidereal, 1 = driveLunar, 2 = driveSolar).
 */
void mount_alpaca_get_tracking_rate_name(int idx, char *buf, size_t len) {
    switch (idx) {
        case 0: snprintf(buf, len, "driveSidereal"); break;
        case 1: snprintf(buf, len, "driveLunar");    break;
        case 2: snprintf(buf, len, "driveSolar");    break;
        default: buf[0] = '\0';                      break;
    }
}

/* ═══════════════════════════════════════════════════════════════
 * Slew / sync
 *
 * Thin wrappers that map Alpaca coordinate calls directly to the
 * mount public API. RA is in hours, DEC is in degrees.
 * ═══════════════════════════════════════════════════════════════ */

MountResult mount_alpaca_slew_to_coordinates(float ra_hours, float dec_deg) {
    return mount_goto(ra_hours, dec_deg, 2);
}

MountResult mount_alpaca_sync_to_coordinates(float ra_hours, float dec_deg) {
    return mount_sync(ra_hours, dec_deg);
}

MountResult mount_alpaca_abort_slew(void) {
    return mount_stop();
}

/* ═══════════════════════════════════════════════════════════════
 * Target coordinates
 *
 * Stored in AlpacaState. The Alpaca protocol separates "set target"
 * from "slew to target" — clients set TargetRightAscension /
 * TargetDeclination first, then call slewtotarget / synctotarget.
 * ═══════════════════════════════════════════════════════════════ */

float mount_alpaca_get_target_ra(void)  { return AlpacaState.target_ra; }
float mount_alpaca_get_target_dec(void) { return AlpacaState.target_dec; }

void mount_alpaca_set_target_ra(float ra_hours) { AlpacaState.target_ra = ra_hours; }
void mount_alpaca_set_target_dec(float dec_deg)  { AlpacaState.target_dec = dec_deg; }

MountResult mount_alpaca_slew_to_target(void) {
    if (AlpacaState.target_ra == 0.0f && AlpacaState.target_dec == 0.0f)
        return mount_result_error("Target not set");
    return mount_goto(AlpacaState.target_ra, AlpacaState.target_dec, 2);
}

MountResult mount_alpaca_sync_to_target(void) {
    if (AlpacaState.target_ra == 0.0f && AlpacaState.target_dec == 0.0f)
        return mount_result_error("Target not set");
    return mount_sync(AlpacaState.target_ra, AlpacaState.target_dec);
}

/* ═══════════════════════════════════════════════════════════════
 * Pier side
 *
 * ASCOM convention: 0 = pierEast (telescope on east side of pier,
 * pointing west), 1 = pierWest.  Stored in AlpacaState — purely
 * informational for clients, the mount does not use it internally.
 * ═══════════════════════════════════════════════════════════════ */

int mount_alpaca_get_side_of_pier(void) {
    return AlpacaState.side_of_pier;
}

MountResult mount_alpaca_set_side_of_pier(int side) {
    if (side != 0 && side != 1)
        return mount_result_error("Invalid pier side");
    AlpacaState.side_of_pier = side;
    return mount_result_ok("Pier side set");
}

/* ═══════════════════════════════════════════════════════════════
 * Slew settle time
 *
 * Time (seconds) the mount should wait after a slew completes before
 * reporting Slewing = false.  Currently stored but not enforced by
 * the motion task — clients may poll slewing until it clears.
 * ═══════════════════════════════════════════════════════════════ */

int mount_alpaca_get_slew_settle_time(void) {
    return AlpacaState.slew_settle_time;
}

MountResult mount_alpaca_set_slew_settle_time(int seconds) {
    AlpacaState.slew_settle_time = seconds;
    return mount_result_ok("Slew settle time set");
}

/* ═══════════════════════════════════════════════════════════════
 * Park position
 *
 * Stores the current axis position as the park position.  The actual
 * park/unpark motion is handled by mount_park() / mount_unpark().
 * This just records "where we are right now" as the park target.
 * ═══════════════════════════════════════════════════════════════ */

MountResult mount_alpaca_set_park(void) {
    MotorsState s = motors_current_state();
    AlpacaState.park_ra_deg = s.ra_position;
    AlpacaState.park_dec_deg = s.dec_position;
    return mount_result_ok("Park position stored");
}
