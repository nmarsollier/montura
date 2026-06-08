/* Motors - motors_slew_axis.c
 *
 * Purpose: accept relative slew requests for a single axis.
 */
#include "motors.h"

#include "esp_log.h"
#include "motors_internal.h"
#include "motors_motion.h"

static const char *TAG = "MOTORS_SLEW_AXIS";

/*
 * Validate a relative axis move request and delegate to the motion subsystem
 * via the priority-aware command queue.
 */
MotorResultCode motors_slew_axis_ra(float degrees, int speed) {
    float actual_speed = motors_get_slewing_speed(speed);

    /* Validate against current position (soft gate — approximate). */
    float future_target = motors_state.ra_position + degrees;
    if (!motors_is_valid_ra(future_target)) {
        ESP_LOGW(TAG, "Rejected RA move: out of range (%.3f)", future_target);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_set_axis_velocity_ra(actual_speed);

    /* Soft gate — visible to status readers immediately. */
    motors_state.status = MOUNT_STATUS_SLEWING;
    motors_state.tracking = TRACKING_NONE;

    /*
     * Send the RELATIVE delta — the motion task computes the absolute target
     * from the live position when the command is dequeued, guaranteeing correct
     * stacking of queued moves.
     */
    motors_motion_slew_relative(degrees, 0.0f, actual_speed, motors_state.dec_velocity);

    return MOTOR_OK;
}

MotorResultCode motors_slew_axis_dec(float degrees, int speed) {
    float actual_speed = motors_get_slewing_speed(speed);

    float future_target = motors_state.dec_position + degrees;
    if (!motors_is_valid_dec(future_target)) {
        ESP_LOGW(TAG, "Rejected DEC move: out of range (%.3f)", future_target);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_set_axis_velocity_dec(actual_speed);

    motors_state.status = MOUNT_STATUS_SLEWING;
    motors_state.tracking = TRACKING_NONE;

    motors_motion_slew_relative(0.0f, degrees, motors_state.ra_velocity, actual_speed);

    return MOTOR_OK;
}
