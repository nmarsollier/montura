/* Buttons - buttons_stop_handler.c
 *
 * Purpose: handle the physical STOP button behaviour.
 *
 * State machine:
 *   DISABLED → enable
 *   not READY → stop
 *   READY     → disable
 */
#include "buttons.h"
#include "motors.h"

#include "esp_log.h"

static const char *TAG = "BUTTONS_STOP";

void buttons_handle_stop(void) {
    MotorsState state = motors_current_state();

    if (state.status == MOTORS_STATUS_DISABLED) {
        motors_enable();
        return;
    }

    if (state.status != MOTORS_STATUS_READY) {
        ESP_LOGI(TAG, "STOP: not READY (status=%d) -> motors_stop()", state.status);
        motors_stop();
        return;
    }

    motors_disable();
}
