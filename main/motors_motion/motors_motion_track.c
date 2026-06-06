/* MotorsMotion - motors_motion_track.c
 *
 * Purpose: request continuous tracking for the selected mode.
 * Sends a high-priority TRACK command that preempts any in-progress slew.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_TRACK";

void motors_motion_track(TrackingMode mode, float ra_vel) {
    MotionCommand cmd = {
        .type = MOTION_CMD_TRACK,
        .ra_target_deg = 0.0f,   /* set by the task from limits */
        .dec_target_deg = 0.0f,  /* set by the task */
        .ra_velocity = ra_vel,
        .dec_velocity = 0.0f,
        .tracking_mode = mode,
    };
    motors_motion_cmd_send(&cmd, true);  /* high priority — preempts SLEW */
    ESP_LOGI(TAG, "Queued TRACK (high prio): mode=%d vel_ra=%.6f", mode, ra_vel);
}
