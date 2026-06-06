/* Motors - motors_disable.c
 *
 * Purpose: disable motor movement via high-priority command.
 */
#include "motors.h"
#include "motors_motion.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MOTORS_DISABLE";

/*
 * Place the motors subsystem into a disabled state.
 */
void motors_disable(void) {
    /* Stop motion first, then disable hardware. */
    motors_state.status      = MOUNT_STATUS_READY;
    motors_state.tracking    = TRACKING_NONE;
    motors_motion_stop();

    motors_motion_hw_disable();

    motors_state.status      = MOUNT_STATUS_DISABLED;
    motors_state.tracking    = TRACKING_MANUAL;
    motors_state.last_update = esp_timer_get_time();

    motors_motion_disable();

    ESP_LOGI(TAG, "Motors disabled");
}
