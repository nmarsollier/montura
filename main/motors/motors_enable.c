/* Motors - motors_enable.c
 *
 * Purpose: enable motor movement.
 */
#include "motors.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "motors_motion.h"
#include "mount.h"

static const char *TAG = "MOTORS_ENABLE";

/*
 * Bring the motors back to an operational state.
 */
void motors_enable(void) {
    motors_motion_hw_enable();
    motors_state.status = MOUNT_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
    motors_state.last_update = esp_timer_get_time();

    ESP_LOGI(TAG, "Motors enabled");
}
