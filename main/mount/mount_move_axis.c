/* Mount - mount_move_axis.c
 *
 * Purpose: move one axis by a relative amount.
 */
#include "mount.h"
#include "mount_internal.h"

#include <string.h>

#include "esp_log.h"

#include "motors.h"

/*
 * Business use case: move a mount axis by a relative amount of degrees.
 *
 * Objective: let operators adjust a single axis without knowing the absolute
 * current position.
 */
MountResult mount_move_axis(MotorAxis axis, float degrees, int speed) {
    if (degrees == 0.0f) {
        return mount_result_error("Degrees cannot be zero");
    }

    MotorResultCode rc = 0;
    if (axis == MOTOR_AXIS_RA) {
        rc = motors_slew_axis_ra(degrees, speed);
    } else if (axis == MOTOR_AXIS_DEC) {
        rc = motors_slew_axis_dec(degrees, speed);
    }
    if (rc != MOTOR_OK) {
        switch (rc) {
            case MOTOR_ERR_INVALID_AXIS:
                return mount_result_error("Invalid axis");
            case MOTOR_ERR_OUT_OF_RANGE:
                return mount_result_error("Target out of range");
            case MOTOR_ERR_NOT_READY:
                return mount_result_error("Motors not ready");
            case MOTOR_ERR_INTERNAL:
            default:
                return mount_result_error("Motor error");
        }
    }

    return mount_result_ok("Move axis started");
}

/*
 * Business use case: move one or both axes continuously at the given rates.
 *
 * Objective: support Alpaca MoveAxis (joystick-style controls) where the
 * client sends a rate and expects continuous motion until rate = 0.
 */
MountResult mount_move_axis_velocity(float rate_ra, float rate_dec) {
    motors_move_axis_velocity(rate_ra, rate_dec);

    return mount_result_ok("Move axis velocity applied");
}
