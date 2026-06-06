#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "motors/motors.h"

/* Use `MotorAxis` from the motors package for mount axis operations. */

typedef struct {
    float lat;
    float lon;
    int elevation;
} MountSettings;

/* MountResult lives here because mount is the API boundary. */
typedef struct {
    bool ok;
    const char *message;
} MountResult;

/* Coordinate types. */
typedef struct {
    float ra_hours;
    float dec_deg;
} EquatorialCoordinates;

typedef struct {
    float ra_axis_deg;
    float dec_axis_deg;
} AxisCoordinates;

/* Formatted astronomical coordinate types. */
typedef struct {
    int hours;
    int minutes;
    float seconds;
} RaHMS;

typedef struct {
    int sign;       /* +1 or -1 */
    int degrees;
    int minutes;
    float seconds;
} DecDMS;

typedef struct {
    MotorsStatus status;
    TrackingMode tracking;
    RaHMS ra;
    DecDMS dec;
    int64_t last_update;
    MountSettings settings;
} VisibleStatusData;

/* Conversion functions between equatorial and physical axis coordinates. */
AxisCoordinates equatorial_to_axis(EquatorialCoordinates eq);

EquatorialCoordinates axis_to_equatorial(AxisCoordinates axis);

/* Convert decimal RA hours to HMS and DEC degrees to DMS. */
RaHMS ra_hours_to_hms(float ra_hours);

DecDMS dec_deg_to_dms(float dec_deg);

/* Public API */

/*
 * mount_init
 * ----------------
 * Initialize mount module state. Prepares internal structures so the mount
 * can accept high-level commands (goto, tracking, park/unpark). This is a
 * hardware-agnostic initialization point; it does not perform physical moves.
 */
void mount_init(void);

/*
 * mount_get_visible_status_data
 * -----------------------------
 * Return the current status view that the REST/API layer and UI consume.
 * The structure contains motors-derived status, current tracking mode, last
 * known RA/DEC (in axis-space), timestamp and persisted settings.
 */
VisibleStatusData mount_get_visible_status_data(void);

/*
 * mount_set_tracking
 * ------------------
 * Set the mount's tracking mode (sidereal, lunar, solar, manual, none).
 * Parameter: `tracking` - desired TrackingMode. Returns a MountResult that
 * indicates whether the request was accepted and includes a short message.
 */
MountResult mount_set_tracking(TrackingMode tracking);

/*
 * mount_goto
 * ----------------
 * Start an asynchronous slewing operation to the requested equatorial
 * coordinates. Parameters:
 *  - `ra` (hours): right ascension in hours.
 *  - `dec` (degrees): declination in degrees.
 *  - `speed` (arbitrary int): requested speed profile (module may ignore).
 * Returns a MountResult describing acceptance or rejection.
 */
MountResult mount_goto(float ra, float dec, int speed);

/*
 * mount_stop
 * ----------
 * Stop any ongoing motion immediately and leave the mount in the READY
 * state. Returns a MountResult indicating success or reason for failure.
 */
MountResult mount_stop(void);

/*
 * mount_park
 * ----------
 * Move the mount to its defined parking position and mark it as PARKED.
 * This is a high-level operation (may be asynchronous); the result reports
 * whether the request was accepted.
 */
MountResult mount_park(void);

/*
 * mount_unpark
 * ------------
 * Take the mount out of the parked state and prepare it for motion. Returns
 * a MountResult describing the outcome.
 */
MountResult mount_unpark(void);

/*
 * mount_sync
 * ----------
 * Set the mount's authoritative position to the provided equatorial
 * coordinates without moving the hardware. Parameters are RA (hours) and
 * DEC (degrees). Use this to correct the internal model after manual
 * adjustments or plate-solving.
 */
MountResult mount_sync(float ra, float dec);

/*
 * mount_update_settings
 * ---------------------
 * Persist and apply user-provided mount settings (location and elevation).
 * Parameter: `settings` - settings structure with latitude, longitude and
 * elevation. Returns success/failure.
 */
MountResult mount_update_settings(MountSettings settings);

MountResult mount_set_system_time(const char *iso_time);

/*
 * Move the mount to its home position.
 */
MountResult mount_home(void);

/*
 * mount_move_axis
 * ---------------
 * Request a small relative move on a single physical axis. Parameters:
 *  - `axis`: which axis to move (RA or DEC).
 *  - `degrees`: delta in degrees to move (positive/negative allowed).
 *  - `speed`: requested speed profile (module may enforce limits).
 * This is the public single-axis move API and returns acceptance info.
 */
MountResult mount_move_axis(MotorAxis axis, float degrees, int speed);

/*
 * mount_move_axis_velocity
 * ------------------------
 * Move one or both axes continuously at the given rates in deg/s until
 * a subsequent call with both rates = 0 (or STOP / PARK) halts motion.
 * Positive = forward, negative = reverse.  Used by Alpaca MoveAxis and
 * manual centering controls.
 */
MountResult mount_move_axis_velocity(float rate_ra, float rate_dec);

/* Convert RA from HMS struct to decimal hours. */
float mount_get_ra_hours(void);

/* Convert DEC from DMS struct to decimal degrees. */
float mount_get_dec_deg(void);

TrackingMode tracking_from_string(const char *value);

/* ─── Alpaca bridge — public API for the Alpaca REST layer ─── */

/* Horizontal coordinates (calculated from equatorial + time + site). */
float mount_alpaca_get_altitude(void);
float mount_alpaca_get_azimuth(void);

/* Sidereal time in hours at the current site and UTC time. */
float mount_alpaca_get_sidereal_time(void);

/* UTC date as ISO 8601 string ("2025-06-05T20:15:00"). */
const char *mount_alpaca_get_utc_date(void);
MountResult  mount_alpaca_set_utc_date(const char *iso_time);

/* State queries. */
bool mount_alpaca_get_is_slewing(void);
bool mount_alpaca_get_is_tracking(void);
bool mount_alpaca_get_is_parked(void);
bool mount_alpaca_get_is_home(void);

/* Site location (degrees, metres). */
float mount_alpaca_get_site_latitude(void);
float mount_alpaca_get_site_longitude(void);
int   mount_alpaca_get_site_elevation(void);
MountResult mount_alpaca_set_site_latitude(float lat);
MountResult mount_alpaca_set_site_longitude(float lon);
MountResult mount_alpaca_set_site_elevation(int elevation);

/* Tracking rate as Alpaca DriveRates enum (0=sidereal, 1=lunar, 2=solar, 3=king). */
int  mount_alpaca_get_tracking_rate(void);
MountResult mount_alpaca_set_tracking_rate(int rate);
bool mount_alpaca_get_tracking(void);
MountResult mount_alpaca_set_tracking(bool enabled);
void mount_alpaca_get_tracking_rate_name(int idx, char *buf, size_t len);

/* Slew / sync in equatorial coordinates. */
MountResult mount_alpaca_slew_to_coordinates(float ra_hours, float dec_deg);
MountResult mount_alpaca_sync_to_coordinates(float ra_hours, float dec_deg);
MountResult mount_alpaca_abort_slew(void);

/* Target coordinates (stored for slewtotarget / synctotarget). */
float mount_alpaca_get_target_ra(void);
float mount_alpaca_get_target_dec(void);
void  mount_alpaca_set_target_ra(float ra_hours);
void  mount_alpaca_set_target_dec(float dec_deg);
MountResult mount_alpaca_slew_to_target(void);
MountResult mount_alpaca_sync_to_target(void);

/* Pier side (0=East, 1=West). */
int  mount_alpaca_get_side_of_pier(void);
MountResult mount_alpaca_set_side_of_pier(int side);

/* Slew settle time (seconds). */
int  mount_alpaca_get_slew_settle_time(void);
MountResult mount_alpaca_set_slew_settle_time(int seconds);

/* Park position. */
MountResult mount_alpaca_set_park(void);