#include "motors.h"
#include "motors_internal.h"
#include "motors_motion.h"

#include "esp_log.h"
#include "esp_timer.h"
/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement.
 */

static const char *TAG = "MOTORS_STOP";

/*
 * Stop both axes immediately and return the mount to READY.
 * The motion task observes the status change and exits its inner loop.
 */
void motors_stop(void) {
    if (motors_state.status == MOUNT_STATUS_DISABLED) {
        motors_enable();
    }

    motors_state.status = MOUNT_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
    motors_state.last_update = esp_timer_get_time();

    ESP_LOGI(TAG, "Motion stopped");
}
