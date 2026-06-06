/* Motors - motors_button_stop.c
 *
 * Purpose: handle STOP button behavior inside the motors module.
 */
#include "motors.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_BUTTON_STOP";

void motors_button_stop(void) {
    MotorsState state = motors_current_state();
    ESP_LOGI(TAG, "STOP invoked: status=%d tracking=%d", state.status, state.tracking);

    if (state.status != MOUNT_STATUS_READY && state.tracking != TRACKING_MANUAL) {
        ESP_LOGI(TAG, "STOP invoked: motors not READY (status=%d) -> motors_stop()", state.status);
        motors_stop();
        return;
    }

    if (state.tracking == TRACKING_MANUAL) {
        ESP_LOGI(TAG, "STOP invoked: READY + TRACKING_MANUAL -> switching to TRACKING_NONE");
        motors_enable();
    } else {
        ESP_LOGI(TAG, "STOP invoked: READY + TRACKING_NONE -> switching to TRACKING_MANUAL");
        motors_disable();
    }
}
