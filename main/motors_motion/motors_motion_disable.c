/* MotorsMotion - motors_motion_disable.c
 *
 * Purpose: request motor disable — stops motion and disables hardware.
 * Sends a maximum-priority DISABLE command.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_DISABLE";

void motors_motion_disable(void) {
    MotionCommand cmd = {
        .type = MOTION_CMD_DISABLE,
        .tracking_mode = TRACKING_NONE,
    };
    motors_motion_cmd_send(&cmd, true);  /* high priority */
    ESP_LOGI(TAG, "Queued DISABLE (high prio)");
}
