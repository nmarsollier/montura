/* MotorsMotion - motors_motion_slew.c
 *
 * Purpose: request a slew to absolute axis targets at the given velocities.
 * Sends a normal-priority SLEW command to the motion task queue.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_SLEW";

void motors_motion_slew(float ra_target, float dec_target,
                         float ra_vel, float dec_vel) {
    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_target_deg = ra_target,
        .dec_target_deg = dec_target,
        .ra_velocity = ra_vel,
        .dec_velocity = dec_vel,
        .tracking_mode = TRACKING_NONE,
    };
    motors_motion_cmd_send(&cmd, false);
    ESP_LOGI(TAG, "Queued SLEW: RA=%.4f DEC=%.4f vel_ra=%.4f vel_dec=%.4f",
             ra_target, dec_target, ra_vel, dec_vel);
}
