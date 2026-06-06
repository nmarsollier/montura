/* Motors - motors_park.c
 *
 * Purpose: park both axes via high-priority command.
 */
#include "motors.h"
#include "motors_motion.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MOTORS_PARK";

void motors_park(void) {
    motors_state.status      = MOUNT_STATUS_PARKED;
    motors_state.tracking    = TRACKING_NONE;
    motors_state.last_update = esp_timer_get_time();

    motors_motion_park();
    ESP_LOGI(TAG, "Park command sent");
}
