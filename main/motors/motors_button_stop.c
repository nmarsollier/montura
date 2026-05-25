/* Motors - motors_button_stop.c
 *
 * Purpose: handle STOP button behavior inside the motors module.
 */
#include "motors.h"

#include "esp_log.h"
#include "mount.h"

static const char *TAG = "MOTORS_BUTTON_STOP";

void motors_button_stop(void) {
    VisibleStatusData vsd = mount_get_visible_status_data();
    ESP_LOGI(TAG, "STOP invoked", vsd.status);

    if (vsd.status != MOUNT_STATUS_READY && vsd.tracking != TRACKING_MANUAL) {
        ESP_LOGI(TAG, "STOP invoked: motors not READY (status=%d) -> motors_stop()", vsd.status);
        motors_stop();
        return;
    }

    if (vsd.tracking == TRACKING_MANUAL) {
        ESP_LOGI(TAG, "STOP invoked: READY + TRACKING_MANUAL -> switching to TRACKING_NONE");
        motors_enable();
    } else {
        ESP_LOGI(TAG, "STOP invoked: READY + TRACKING_NONE -> switching to TRACKING_MANUAL");
        motors_disable();
    }
}
