/* Motors - motors_get_tracking_speed.c
 *
 * Purpose: return the angular speed for a given tracking mode.
 */
#include "motors.h"

/*
 * Tracking profiles map to physical axis angular speeds in degrees per second.
 *
 * Mechanical gearing affects step rates, but the public API still works in
 * axis space.
 */

#define SIDEREAL_RATE_DEG_PER_S (360.0f / 86164.0905308f)

float motors_get_tracking_speed(TrackingMode mode) {
    switch (mode) {
        case TRACKING_SIDEREAL:
            return SIDEREAL_RATE_DEG_PER_S;

        case TRACKING_SOLAR: {
            /* Sun's mean eastward motion among the stars. */
            const float SIDEREAL_YEAR_DAYS = 365.256363004f;
            const float sun_rate = 360.0f / (SIDEREAL_YEAR_DAYS * 86400.0f); /* deg/s */
            return SIDEREAL_RATE_DEG_PER_S - sun_rate;
        }

        case TRACKING_LUNAR: {
            /* Moon's mean eastward motion among the stars. */
            const float SIDEREAL_MONTH_DAYS = 27.321661f;
            const float moon_rate = 360.0f / (SIDEREAL_MONTH_DAYS * 86400.0f); /* deg/s */
            return SIDEREAL_RATE_DEG_PER_S - moon_rate;
        }

        case TRACKING_MANUAL:
        case TRACKING_NONE:
        default:
            return 0.0f;
    }
}
