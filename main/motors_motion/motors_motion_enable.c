/* MotorsMotion - motors_motion_enable.c
 *
 * Purpose: request motor enable — brings motors back to operational state.
 * Sends a normal-priority ENABLE command.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_ENABLE";

void motors_motion_enable(void) {
    MotionCommand cmd = {
        .type = MOTION_CMD_ENABLE,
        .tracking_mode = TRACKING_NONE,
    };
    motors_motion_cmd_send(&cmd, false);
    ESP_LOGI(TAG, "Queued ENABLE");
}
