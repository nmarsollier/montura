/* MotorsMotion - motors_motion_park.c
 *
 * Purpose: request park state — stops motion and marks the mount as parked.
 * Sends a maximum-priority PARK command.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_PARK";

void motors_motion_park(void) {
    MotionCommand cmd = {
        .type = MOTION_CMD_PARK,
        .tracking_mode = TRACKING_NONE,
    };
    motors_motion_cmd_send(&cmd, true);
    ESP_LOGI(TAG, "Queued PARK (high prio)");
}
