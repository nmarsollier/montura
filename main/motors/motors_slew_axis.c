/* Motors - motors_slew_axis.c
 *
 * Purpose: accept relative slew requests for a single axis.
 *
 * The motion task computes the absolute target from the live axis
 * position when the command is dequeued, guaranteeing correct
 * stacking of queued moves.
 */
#include "motors.h"
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_SLEW_AXIS";

MotorResultCode motors_slew_axis_ra(float degrees, int speed, float lat) {
    TrackingMode currTracking = TRACKING_NONE;
    if (motors_state.status == MOTORS_STATUS_TRACKING
        && motors_state.tracking != TRACKING_NONE) {
        currTracking = motors_state.tracking;
        motors_stop();
    }

    float actual_speed = motors_get_slewing_speed(speed);

    float future_target = motors_state.ra_position + degrees;
    if (!motors_is_valid_ra(future_target)) {
        ESP_LOGW(TAG, "Rejected RA move: out of range (%.3f)", future_target);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_state.ra_velocity = actual_speed;

    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_velocity = actual_speed,
        .dec_velocity = motors_state.dec_velocity,
        .relative = true,
        .ra_delta_deg = degrees,
        .dec_delta_deg = 0.0f,
    };
    motors_queue_send(&cmd);

    if (currTracking != TRACKING_NONE) {
        motors_start_tracking(currTracking, lat);
    }
    return MOTOR_OK;
}

MotorResultCode motors_slew_axis_dec(float degrees, int speed, float lat) {
    TrackingMode currTracking = TRACKING_NONE;
    if (motors_state.status == MOTORS_STATUS_TRACKING
        && motors_state.tracking != TRACKING_NONE) {
        currTracking = motors_state.tracking;
        motors_stop();
    }

    float actual_speed = motors_get_slewing_speed(speed);

    float future_target = motors_state.dec_position + degrees;
    if (!motors_is_valid_dec(future_target)) {
        ESP_LOGW(TAG, "Rejected DEC move: out of range (%.3f)", future_target);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_state.dec_velocity = actual_speed;

    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_velocity = motors_state.ra_velocity,
        .dec_velocity = actual_speed,
        .relative = true,
        .ra_delta_deg = 0.0f,
        .dec_delta_deg = degrees,
    };
    motors_queue_send(&cmd);

    if (currTracking != TRACKING_NONE) {
        motors_start_tracking(currTracking, lat);
    }

    return MOTOR_OK;
}
