/* Motors - motors_sync_position.c
 *
 * Purpose: update the authoritative axis position model via command queue.
 */
#include "motors.h"
#include "motors_internal.h"
#include "motors_motion.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_SYNC_POSITION";

void motors_sync_position(float ra_axis_deg, float dec_axis_deg) {
    motors_motion_sync(ra_axis_deg, dec_axis_deg);

    ESP_LOGI(TAG, "Sync command sent: RA=%.3f DEC=%.3f",
             ra_axis_deg, dec_axis_deg);
}
