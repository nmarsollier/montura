/* Motors - motors_home.c
 *
 * Purpose: move the mount to the home position.
 */
#include "motors.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_HOME";

void motors_home(void) {
    ESP_LOGI(TAG, "Home requested: stopping and slewing to home angles");
    motors_stop();

    motors_slew_to_angle(0.0f, 0.0f, 0);
}
