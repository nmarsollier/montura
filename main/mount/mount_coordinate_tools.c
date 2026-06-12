/* Mount - mount_coordinates.c
 *
 * Purpose: convert between physical axis coordinates and equatorial
 * coordinates.
 */
#include "mount.h"
#include "mount_internal.h"

#include <math.h>
#include <time.h>
#include <string.h>

/*
 * Mapping notes:
 * - Convert RA/DEC to hour angle using local sidereal time.
 * - `mount_internal_state.lon` is stored in degrees, positive east.
 * - Axis zero is defined as the mechanical home position, so we apply fixed
 *   offsets to map between hardware angles and sky coordinates.
 */

static double unix_time_to_julian_date(time_t t) {
    /* Convert Unix epoch seconds to Julian Date. */
    return (double) t / 86400.0 + 2440587.5;
}

static double gmst_hours_from_jd(double jd) {
    /* Compute GMST in hours from Julian Date using an approximate formula. */
    double T = (jd - 2451545.0) / 36525.0;
    double gmst_deg = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * T * T - (T * T * T) /
                      38710000.0;
    /* Normalize to 0..360. */
    gmst_deg = fmod(gmst_deg, 360.0);
    if (gmst_deg < 0.0)
        gmst_deg += 360.0;
    return gmst_deg / 15.0; /* hours */
}

static float normalize_degf(float d) {
    d = fmodf(d, 360.0f);
    if (d > 180.0f) d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    return d;
}

static float normalize_hoursf(float h) {
    while (h < 0.0f)
        h += 24.0f;
    while (h >= 24.0f)
        h -= 24.0f;
    return h;
}

/*
 * --------------------------------------------------------------------------
 * Equatorial ↔ Axis coordinate conversion.
 *
 * Geometry (German equatorial mount, southern hemisphere, home = SCP):
 *
 *   RA axis  = hour-angle-driven polar rotation.
 *              Home (0°) = meridian.   +right / −left from home.
 *              Physical limit: ±120° (tripod collision).
 *
 *   DEC axis = declination tilt relative to RA axis.
 *              Home (0°) = SCP (DEC = −90°).
 *              DEC = 0° (equator) = axis 90°.
 *              DEC = +90° (NCP) = axis 180°.
 *              Physical limit: ±180° (cable wrap).
 *
 * Pier side / meridian flip:
 *
 *   A GEM can reach the same sky position from two mechanical
 *   configurations.  We always prefer the side that keeps RA within
 *   its physical limits (±120°).
 *
 *   Normal side (no flip):
 *     DEC_axis = DEC + 90°
 *     RA_axis  = HA × 15          (HA = LST − RA, wrapped to [−12h, +12h])
 *
 *   Flipped side (pier flip, used when |RA_axis| > 120°):
 *     DEC_axis = −(DEC + 90°)            (negate the normal DEC)
 *     RA_axis  = RA_axis_normal ± 180°   (whichever stays within limits)
 *
 *   The flipped DEC is simply the negative of the normal DEC — the
 *   telescope tube tilts the same amount but in the opposite direction
 *   because the RA axis has rotated 180° to the other side of the pier.
 * -------------------------------------------------------------------------- */

#define RA_LIMIT_DEG 120.0f

AxisCoordinates equatorial_to_axis(EquatorialCoordinates eq) {
    AxisCoordinates out;

    /* Compute local sidereal time. */
    time_t now = time(NULL);
    double jd = unix_time_to_julian_date(now);
    double gmst_h = gmst_hours_from_jd(jd);
    double lon = mount_internal_state.lon;
    double lst_h = gmst_h + (lon / 15.0);

    /* Hour angle = LST − RA.  Wrap to [−12h, +12h] so we pick the
     * shortest path to the target.  Positive = west, negative = east. */
    float ha_h = (float) (lst_h - eq.ra_hours);
    while (ha_h > 12.0f)  ha_h -= 24.0f;
    while (ha_h < -12.0f) ha_h += 24.0f;

    float ha_deg = ha_h * 15.0f;           /* [−180°, +180°] */
    float ra_axis = ha_deg;                /* normal side */

    /* Decide pier side based on RA physical limits. */
    bool flipped = false;
    if (ra_axis > RA_LIMIT_DEG) {
        ra_axis -= 180.0f;
        flipped = true;
    } else if (ra_axis < -RA_LIMIT_DEG) {
        ra_axis += 180.0f;
        flipped = true;
    }

    out.ra_axis_deg = normalize_degf(ra_axis);

    if (flipped) {
        /* Flipped: DEC axis is the negative of the normal value.
         * The RA axis rotated 180°, so the DEC tilt direction reverses. */
        out.dec_axis_deg = normalize_degf(-(eq.dec_deg + 90.0f));
    } else {
        /* Normal:  DEC_axis = DEC_sky + 90° (SCP at 0°) */
        out.dec_axis_deg = normalize_degf(eq.dec_deg + 90.0f);
    }

    return out;
}

EquatorialCoordinates axis_to_equatorial(AxisCoordinates axis) {
    EquatorialCoordinates out;

    /* Detect flip from DEC axis sign.
     *
     * equatorial_to_axis produces:
     *   Normal:  dec_axis =  DEC + 90°  →  dec_axis ∈ [0°, 180°]
     *   Flipped: dec_axis = −(DEC + 90°)  →  dec_axis ∈ [−180°, 0°)
     *
     * The sign of the current DEC axis position is deterministic —
     * unlike RA, which stays within ±RA_LIMIT_DEG after a flip and
     * cannot be used to distinguish the two configurations.
     */
    float ha_deg;
    if (axis.dec_axis_deg >= 0.0f) {
        /* Normal side — RA axis directly encodes HA. */
        ha_deg = normalize_degf(axis.ra_axis_deg);
        out.dec_deg = axis.dec_axis_deg - 90.0f;
    } else {
        /* Flipped side — reverse the flip.
         * DEC_axis = −(DEC_sky + 90°)  →  DEC_sky = −DEC_axis − 90° */
        ha_deg = normalize_degf(axis.ra_axis_deg > 0.0f
                                    ? axis.ra_axis_deg - 180.0f
                                    : axis.ra_axis_deg + 180.0f);
        out.dec_deg = -axis.dec_axis_deg - 90.0f;
    }

    float ha_h = ha_deg / 15.0f;

    /* Recompute LST. */
    time_t now = time(NULL);
    double jd = unix_time_to_julian_date(now);
    double gmst_h = gmst_hours_from_jd(jd);
    double lon = mount_internal_state.lon;
    double lst_h = gmst_h + (lon / 15.0);

    out.ra_hours = normalize_hoursf((float) (lst_h - ha_h));

    return out;
}

RaHMS ra_hours_to_hms(float ra_hours) {
    RaHMS out;
    float total = fabsf(ra_hours);

    out.hours = (int) total;
    float rem = (total - (float) out.hours) * 60.0f;
    out.minutes = (int) rem;
    out.seconds = (rem - (float) out.minutes) * 60.0f;

    return out;
}

DecDMS dec_deg_to_dms(float dec_deg) {
    DecDMS out;
    out.sign = (dec_deg >= 0.0f) ? 1 : -1;
    float total = fabsf(dec_deg);

    out.degrees = (int) total;
    float rem = (total - (float) out.degrees) * 60.0f;
    out.minutes = (int) rem;
    out.seconds = (rem - (float) out.minutes) * 60.0f;

    return out;
}
