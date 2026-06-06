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
  if (motors_state.status != MOUNT_STATUS_READY) {
    ESP_LOGW(TAG, "Rejected move: motors not ready (status=%d)", motors_state.status);
    return MOTOR_ERR_NOT_READY;
  }

  float actual_speed = motors_get_slewing_speed(speed);

  float target = motors_state.ra_position + degrees;
  if (!motors_is_valid_ra(target)) {
    ESP_LOGW(TAG, "Rejected RA move: out of range (%.3f)", target);
    return MOTOR_ERR_OUT_OF_RANGE;
  }
  motors_set_axis_velocity_ra(actual_speed);

  /* Soft gate — visible to status readers immediately. */
  motors_state.status   = MOUNT_STATUS_SLEWING;
  motors_state.tracking = TRACKING_NONE;

  motors_motion_slew(target, motors_state.dec_position,
                             actual_speed, motors_state.dec_velocity);
  ESP_LOGI(TAG, "Slew RA by %.3f -> target=%.3f (speed=%.6f)", degrees, target, actual_speed);

  return MOTOR_OK;
}

MotorResultCode motors_slew_axis_dec(float degrees, int speed) {
  if (motors_state.status != MOUNT_STATUS_READY) {
    ESP_LOGW(TAG, "Rejected move: motors not ready (status=%d)", motors_state.status);
    return MOTOR_ERR_NOT_READY;
  }

  float actual_speed = motors_get_slewing_speed(speed);

  float target = motors_state.dec_position + degrees;
  if (!motors_is_valid_dec(target)) {
    ESP_LOGW(TAG, "Rejected DEC move: out of range (%.3f)", target);
    return MOTOR_ERR_OUT_OF_RANGE;
  }
  motors_set_axis_velocity_dec(actual_speed);

  /* Soft gate — visible to status readers immediately. */
  motors_state.status   = MOUNT_STATUS_SLEWING;
  motors_state.tracking = TRACKING_NONE;

  motors_motion_slew(motors_state.ra_position, target,
                             motors_state.ra_velocity, actual_speed);
  ESP_LOGI(TAG, "Slew DEC by %.3f -> target=%.3f (speed=%.6f)", degrees, target, actual_speed);

  return MOTOR_OK;
}
