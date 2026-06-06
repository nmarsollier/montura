/* Motors - motors_move_axis_velocity.c
 *
 * Purpose: request continuous axis velocity motion.
 * Delegates to the motors_motion command queue.
 */
#include "motors.h"
#include "motors_motion.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOVE_AXIS_VELOCITY";

void motors_move_axis_velocity(float rate_ra, float rate_dec) {
    motors_motion_move_axis(rate_ra, rate_dec);
    ESP_LOGI(TAG, "Move axis velocity: RA=%.6f DEC=%.6f deg/s", rate_ra, rate_dec);
}
