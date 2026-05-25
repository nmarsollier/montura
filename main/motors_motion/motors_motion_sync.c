/* MotorsMotion - motors_motion_sync.c
 *
 * Purpose: align motion targets with the authoritative axis positions.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_MOTION_SYNC";

void motors_motion_sync(float ra_axis_deg, float dec_axis_deg)
{
  motors_motion_target_ra_deg  = ra_axis_deg;
  motors_motion_target_dec_deg = dec_axis_deg;
  ESP_LOGI(TAG, "Motion targets synced: RA=%.4f DEC=%.4f", ra_axis_deg, dec_axis_deg);
}
