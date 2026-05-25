/* Motors - motors_init.c
 *
 * Purpose: initialize the motors module and its default state.
 */
#include "motors.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "motors_internal.h"
#include "motors_motion.h"

static const char *TAG = "MOTORS_INIT";

/*
 * Default motors state.
 *
 * Positions are expressed in degrees and use the -180..180 range.
 * The initial value is 0.0, which represents the home/alignment reference.
 */
MotorsState motors_state = {
    .ra_position = 0.0f,
    .dec_position = 0.0f,
    .status = MOUNT_STATUS_READY,
    .tracking = TRACKING_NONE,
    .ra_velocity = 0.0f,
    .dec_velocity = 0.0f,
    .last_update = 0,
    .limits = {
        .ra_min = -180.0f,
        .ra_max = 180.0f,
        .dec_min = -120.0f,
        .dec_max = 120.0f,
    },
};

void motors_init(void) {
    /* Keep this entry point for future hardware initialization. */
    motors_motion_init();
    motors_state.last_update = esp_timer_get_time();
    ESP_LOGI(TAG, "Motors initialized (RA=%.3f DEC=%.3f)", motors_state.ra_position, motors_state.dec_position);
}
