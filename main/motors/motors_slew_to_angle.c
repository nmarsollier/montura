/* Motors - motors_slew_to_angle.c
 *
 * Purpose: public API for absolute axis moves.
 */
#include "motors.h"

#include "esp_log.h"
#include "motors_internal.h"
#include "motors_motion.h"

static const char *TAG = "MOTORS_SLEW_TO_ANGLE";

MotorResultCode motors_slew_axis_to_angle_ra(float degrees, float speed) {
    if (motors_state.status != MOUNT_STATUS_READY) {
        ESP_LOGW(TAG, "Rejected move-to-angle: motors not ready (status=%d)", motors_state.status);
        return MOTOR_ERR_NOT_READY;
    }

    float actual_speed = motors_get_slewing_speed((int) speed);

    if (!motors_is_valid_ra(degrees)) {
        ESP_LOGW(TAG, "Rejected RA move: out of range (%.3f)", degrees);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_set_axis_velocity_ra(actual_speed);

    motors_state.status   = MOUNT_STATUS_SLEWING;
    motors_state.tracking = TRACKING_NONE;

    motors_motion_slew(degrees, motors_state.dec_position,
                               actual_speed, motors_state.dec_velocity);
    ESP_LOGI(TAG, "Slew to angle RA: target=%.3f speed=%.6f", degrees, actual_speed);
    return MOTOR_OK;
}

MotorResultCode motors_slew_axis_to_angle_dec(float degrees, float speed) {
    if (motors_state.status != MOUNT_STATUS_READY) {
        ESP_LOGW(TAG, "Rejected move-to-angle: motors not ready (status=%d)", motors_state.status);
        return MOTOR_ERR_NOT_READY;
    }

    float actual_speed = motors_get_slewing_speed((int) speed);

    if (!motors_is_valid_dec(degrees)) {
        ESP_LOGW(TAG, "Rejected DEC move: out of range (%.3f)", degrees);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_set_axis_velocity_dec(actual_speed);

    motors_state.status   = MOUNT_STATUS_SLEWING;
    motors_state.tracking = TRACKING_NONE;

    motors_motion_slew(motors_state.ra_position, degrees,
                               motors_state.ra_velocity, actual_speed);
    ESP_LOGI(TAG, "Slew to angle DEC: target=%.3f speed=%.6f", degrees, actual_speed);
    return MOTOR_OK;
}

MotorResultCode motors_slew_to_angle(float ra_deg, float dec_deg, float speed) {
    if (motors_state.status != MOUNT_STATUS_READY) {
        ESP_LOGW(TAG, "Rejected move-to-angle: motors not ready (status=%d)", motors_state.status);
        return MOTOR_ERR_NOT_READY;
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

    motors_set_axis_velocity_ra(actual_speed);
    motors_set_axis_velocity_dec(actual_speed);

    motors_state.status   = MOUNT_STATUS_SLEWING;
    motors_state.tracking = TRACKING_NONE;

    motors_motion_slew(ra_deg, dec_deg, actual_speed, actual_speed);

    ESP_LOGI(TAG, "Slew to RA=%.3f DEC=%.3f (speed=%.6f)", ra_deg, dec_deg, actual_speed);
    return MOTOR_OK;
}
