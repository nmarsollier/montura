/* Motors - motors_tools.c
 *
 * Purpose: motors enum/string helpers and state utilities.
 */
#include "motors.h"

#include "esp_timer.h"

#include <stdbool.h>
#include <string.h>

#include "tools/tools.h"

/*
 * Status string helpers live here so the motors module remains the canonical
 * source for mount state names.
 */
const char *status_to_string(MotorsStatus status) {
    switch (status) {
        case MOUNT_STATUS_READY:
            return "ready";
        case MOUNT_STATUS_TRACKING:
            return "tracking";
        case MOUNT_STATUS_DISABLED:
            return "disabled";
        case MOUNT_STATUS_SLEWING:
            return "slewing";
        case MOUNT_STATUS_PARKED:
            return "parked";
        default:
            return "unknown";
    }
}

/*
 * Tracking string helpers live here so REST and UI layers use the same
 * canonical values as the motors module.
 */
const char *tracking_to_string(TrackingMode tracking) {
    switch (tracking) {
        case TRACKING_NONE:
            return "none";
        case TRACKING_MANUAL:
            return "manual";
        case TRACKING_SIDEREAL:
            return "sidereal";
        case TRACKING_LUNAR:
            return "lunar";
        case TRACKING_SOLAR:
            return "solar";
        default:
            return "unknown";
    }
}

TrackingMode tracking_from_string(const char *value) {
    if (value == NULL) {
        return TRACKING_UNKNOWN;
    }

    if (strcmp(value, "none") == 0) {
        return TRACKING_NONE;
    }

    if (strcmp(value, "manual") == 0) {
        return TRACKING_MANUAL;
    }

    if (strcmp(value, "sidereal") == 0) {
        return TRACKING_SIDEREAL;
    }

    if (strcmp(value, "lunar") == 0) {
        return TRACKING_LUNAR;
    }

    if (strcmp(value, "solar") == 0) {
        return TRACKING_SOLAR;
    }

    return TRACKING_UNKNOWN;
}

const char *tracking_valid_values(void) {
    static char buffer[35];
    static bool initialized = false;

    if (initialized) {
        return buffer;
    }

    const char *values[TRACKING_UNKNOWN];

    for (int i = 0; i < TRACKING_UNKNOWN; i++) {
        values[i] = tracking_to_string((TrackingMode) i);
    }

    string_join(buffer, sizeof(buffer), values, TRACKING_UNKNOWN, "|", "[", "]");
    initialized = true;

    return buffer;
}

/*
 * Validate axis positions against the configured mechanical limits.
 */
bool motors_is_valid_ra(float value) {
    return value >= motors_state.limits.ra_min && value <= motors_state.limits.ra_max;
}

bool motors_is_valid_dec(float value) {
    return value >= motors_state.limits.dec_min && value <= motors_state.limits.dec_max;
}

/* Axis string helpers live here so REST and UI layers use the same
 * canonical values as the motors module.
 */
const char *motors_axis_to_string(MotorAxis axis) {
    switch (axis) {
        case MOTOR_AXIS_RA:
            return "ra";
        case MOTOR_AXIS_DEC:
            return "dec";
        default:
            return "unknown";
    }
}

/* Canonical string-to-axis mapping shared by the rest of the codebase. */
MotorAxis motors_axis_from_string(const char *value) {
    if (value == NULL)
        return MOTOR_AXIS_UNKNOWN;
    if (strcmp(value, "ra") == 0)
        return MOTOR_AXIS_RA;
    if (strcmp(value, "dec") == 0)
        return MOTOR_AXIS_DEC;
    return MOTOR_AXIS_UNKNOWN;
}

const char *motors_axis_valid_values(void) {
    static char buffer[15];
    static bool initialized = false;

    if (initialized) {
        return buffer;
    }

    const char *values[MOTOR_AXIS_UNKNOWN];

    for (int i = 0; i < MOTOR_AXIS_UNKNOWN; i++) {
        values[i] = motors_axis_to_string((MotorAxis) i);
    }

    string_join(buffer, sizeof(buffer), values, MOTOR_AXIS_UNKNOWN, "|", "[", "]");
    initialized = true;

    return buffer;
}
