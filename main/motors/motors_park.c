#include "motors.h"
#include "motors_internal.h"
#include "motors_motion.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MOTORS_PARK";
/* Motors - motors_park.c
 *
 * Purpose: park both axes and mark the mount as parked.
 */

/* Set the parked state. The motion task observes the status change and stops. */
void motors_park(void) {
    motors_state.status = MOUNT_STATUS_PARKED;
    motors_state.tracking = TRACKING_NONE;
    motors_state.last_update = esp_timer_get_time();
    ESP_LOGI(TAG, "Motors parked");
}
