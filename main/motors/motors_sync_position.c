/* Motors - motors_sync_position.c
 *
 * Purpose: update the authoritative axis position model.
 */
#include "motors.h"
#include "motors_internal.h"
#include "motors_motion.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MOTORS_SYNC_POSITION";

void motors_sync_position(float ra_axis_deg, float dec_axis_deg) {
    motors_state.ra_position = ra_axis_deg;
    motors_state.dec_position = dec_axis_deg;

    /* Keep motion targets aligned to avoid spurious movement on next start. */
    motors_motion_sync(motors_state.ra_position, motors_state.dec_position);

    motors_state.last_update = esp_timer_get_time();

    ESP_LOGI(TAG, "Motors sync: RA=%.3f DEC=%.3f",
             motors_state.ra_position, motors_state.dec_position);
}
