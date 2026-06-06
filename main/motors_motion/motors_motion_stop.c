/* MotorsMotion - motors_motion_stop.c
 *
 * Purpose: request an immediate stop of all motor movement.
 * Sends a maximum-priority STOP command — preempts everything.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_STOP";

void motors_motion_stop(void) {
    MotionCommand cmd = {
        .type = MOTION_CMD_STOP,
        .tracking_mode = TRACKING_NONE,
    };
    motors_motion_cmd_send(&cmd, true);  /* high priority */
    ESP_LOGI(TAG, "Queued STOP (high prio)");
}
