/* Motors - motors_button_stop.c
 *
 * Purpose: handle STOP button behavior inside the motors module.
 */
#include "motors.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_BUTTON_STOP";

void motors_button_stop(void) {
    MotorsState state = motors_current_state();

    if (state.status == MOUNT_STATUS_DISABLED) {
        motors_enable();
    }

    if (state.status != MOUNT_STATUS_READY) {
        ESP_LOGI(TAG, "STOP invoked: motors not READY (status=%d) -> motors_stop()", state.status);
        motors_stop();
        return;
    }

    motors_disable();
}
