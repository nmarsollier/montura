/* Motors - motors_disable.c
 *
 * Purpose: disable motor movement.
 */
#include "motors.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "motors_motion.h"

static const char *TAG = "MOTORS_DISABLE";

/*
 * Place the motors subsystem into a disabled state.
 */
void motors_disable(void) {
    motors_stop();

    motors_motion_hw_disable();

    motors_state.status = MOUNT_STATUS_DISABLED;
    motors_state.tracking = TRACKING_MANUAL;
    motors_state.last_update = esp_timer_get_time();

    ESP_LOGI(TAG, "Motors disabled");
}
