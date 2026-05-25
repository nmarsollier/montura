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
    if (d < 0.0f)
        d += 360.0f;
    return d;
}

static float normalize_hoursf(float h) {
    while (h < 0.0f)
        h += 24.0f;
    while (h >= 24.0f)
        h -= 24.0f;
    return h;
}

AxisCoordinates equatorial_to_axis(EquatorialCoordinates eq) {
    AxisCoordinates out;

    /* Get current time in UTC and compute local sidereal time. */
    time_t now = time(NULL);
    double jd = unix_time_to_julian_date(now);
    double gmst_h = gmst_hours_from_jd(jd);

    /* Longitude from settings, in degrees and positive east. */
    double lon = mount_internal_state.lon;
    double lst_h = gmst_h + (lon / 15.0);
    lst_h = normalize_hoursf((float) lst_h);

    /* Hour angle in hours = LST - RA. */
    float ha_h = normalize_hoursf((float) (lst_h - eq.ra_hours));
    float ha_deg = ha_h * 15.0f;

    /* Mechanical offsets to map axis-zero conventions. */
    const float RA_AXIS_OFFSET_DEG = 180.0f; /* Zero points to the south pole. */
    const float DEC_AXIS_OFFSET_DEG = 90.0f; /* Mechanical zero equals DEC -90. */

    out.ra_axis_deg = normalize_degf(ha_deg + RA_AXIS_OFFSET_DEG);
    out.dec_axis_deg = normalize_degf(eq.dec_deg + DEC_AXIS_OFFSET_DEG);

    return out;
}

EquatorialCoordinates axis_to_equatorial(AxisCoordinates axis) {
    EquatorialCoordinates out;

    /* Inverse of the above mapping. */
    const float RA_AXIS_OFFSET_DEG = 180.0f;
    const float DEC_AXIS_OFFSET_DEG = 90.0f;

    float ha_deg = normalize_degf(axis.ra_axis_deg - RA_AXIS_OFFSET_DEG);
    float ha_h = ha_deg / 15.0f;

    /* Recompute LST from the current time and site settings. */
    time_t now = time(NULL);
    double jd = unix_time_to_julian_date(now);
    double gmst_h = gmst_hours_from_jd(jd);
    double lon = mount_internal_state.lon;
    double lst_h = gmst_h + (lon / 15.0);
    lst_h = normalize_hoursf((float) lst_h);

    float ra_h = normalize_hoursf(lst_h - ha_h);
    out.ra_hours = ra_h;

    out.dec_deg = axis.dec_axis_deg - DEC_AXIS_OFFSET_DEG;

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
