/* MotorsMotion - motors_motion_move_axis.c
 *
 * Purpose: request continuous single-axis velocity motion.
 * Positive rate = forward, negative = reverse, zero = stop that axis.
 * When both axes reach zero the status returns to READY.
 *
 * Used by Alpaca MoveAxis, physical buttons, joystick, and future guiding.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include <math.h>

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_MOVE_AXIS";

void motors_motion_move_axis(float rate_ra, float rate_dec) {
    /*
     * When both rates are zero this is equivalent to STOP.
     * Use the dedicated stop path for clean status transition.
     */
    if (fabsf(rate_ra) < 1e-9f && fabsf(rate_dec) < 1e-9f) {
        motors_motion_stop();
        return;
    }

    MotionCommand cmd = {
        .type = MOTION_CMD_MOVE_AXIS,
        .ra_target_deg = 0.0f,   /* set by the task from limits */
        .dec_target_deg = 0.0f,  /* set by the task */
        .ra_velocity = rate_ra,
        .dec_velocity = rate_dec,
        .tracking_mode = TRACKING_MANUAL,
    };
    motors_motion_cmd_send(&cmd, false);
    ESP_LOGI(TAG, "Queued MOVE_AXIS: RA=%.6f DEC=%.6f deg/s", rate_ra, rate_dec);
}
