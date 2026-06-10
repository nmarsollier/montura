/* Motors - motors_slew_to_angle.c
 *
 * Purpose: move one or both axes to absolute angles (degrees).
 *
 * When tracking is active the caller pauses it, enqueues a SLEW,
 * and then enqueues a TRACK to resume after the slew completes.
 * Status and tracking are set by process_command() in the motion
 * task — no soft gates needed.
 */
#include "motors.h"
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_SLEW_TO_ANGLE";

MotorResultCode motors_slew_axis_to_angle_ra(float degrees, float speed) {
    float actual_speed = motors_get_slewing_speed((int) speed);

    if (!motors_is_valid_ra(degrees)) {
        ESP_LOGW(TAG, "Rejected RA move: out of range (%.3f)", degrees);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_state.ra_velocity = actual_speed;

    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_target_deg = degrees,
        .dec_target_deg = motors_state.dec_position,
        .ra_velocity = actual_speed,
        .dec_velocity = motors_state.dec_velocity,
    };
    motors_queue_send(&cmd);

    return MOTOR_OK;
}

MotorResultCode motors_slew_axis_to_angle_dec(float degrees, float speed) {
    float actual_speed = motors_get_slewing_speed((int) speed);

    if (!motors_is_valid_dec(degrees)) {
        ESP_LOGW(TAG, "Rejected DEC move: out of range (%.3f)", degrees);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_state.dec_velocity = actual_speed;

    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_target_deg = motors_state.ra_position,
        .dec_target_deg = degrees,
        .ra_velocity = motors_state.ra_velocity,
        .dec_velocity = actual_speed,
    };
    motors_queue_send(&cmd);

    return MOTOR_OK;
}

MotorResultCode motors_slew_to_angle(float ra_deg, float dec_deg, float speed, float lat) {
    TrackingMode currTracking = TRACKING_NONE;
    if (motors_state.status == MOTORS_STATUS_TRACKING
        && motors_state.tracking != TRACKING_NONE) {
        currTracking = motors_state.tracking;
        motors_stop();
    }

    if (!motors_is_valid_ra(ra_deg)) {
        ESP_LOGW(TAG, "Rejected slew: RA out of range (%.3f)", ra_deg);
        return MOTOR_ERR_OUT_OF_RANGE;
    }

    if (!motors_is_valid_dec(dec_deg)) {
        ESP_LOGW(TAG, "Rejected slew: DEC out of range (%.3f)", dec_deg);
        return MOTOR_ERR_OUT_OF_RANGE;
    }

    float actual_speed = motors_get_slewing_speed((int) speed);

    motors_state.ra_velocity = actual_speed;
    motors_state.dec_velocity = actual_speed;

    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_target_deg = ra_deg,
        .dec_target_deg = dec_deg,
        .ra_velocity = actual_speed,
        .dec_velocity = actual_speed,
    };
    motors_queue_send(&cmd);

    if (currTracking != TRACKING_NONE) {
        motors_start_tracking(currTracking, lat);
    }

    return MOTOR_OK;
}
