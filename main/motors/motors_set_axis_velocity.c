/* Motors - motors_set_axis_velocity.c
 *
 * Purpose: update an axis velocity and derive the step rate.
 *
 * Microstep count is obtained at runtime from the TMC2209 driver
 * (the single source of truth) via motors_get_microsteps().
 */
#include "motors.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "motors_internal.h"
#include "motors_motion.h"

static const char *TAG = "MOTORS_SET_AXIS_VELOCITY";

/*
 * Update the commanded velocity for a physical axis.
 * Step rate derivation uses the TMC-verified microstep count.
 */
void motors_set_axis_velocity_ra(float degrees_per_second) {
    motors_state.ra_velocity = degrees_per_second;
    float axis_rev_per_s = degrees_per_second / 360.0f;
    float motor_revs_per_axis_rev = (float) AXIS_PULLEY_TEETH / (float) MOTOR_PULLEY_TEETH;
    int motor_full_steps = MOTOR_FULL_STEPS_PER_REV;
    uint16_t microsteps = motors_get_microsteps();
    float motor_steps_per_axis_rev = motor_full_steps * (float) microsteps * motor_revs_per_axis_rev;

    motors_state.ra_steps_per_s = axis_rev_per_s * motor_steps_per_axis_rev;
    motors_state.last_update = esp_timer_get_time();
    ESP_LOGI(TAG, "Set axis RA velocity = %.6f deg/s -> %.1f steps/s (µsteps=%u)",
             degrees_per_second, motors_state.ra_steps_per_s, microsteps);
}

void motors_set_axis_velocity_dec(float degrees_per_second) {
    motors_state.dec_velocity = degrees_per_second;

    float axis_rev_per_s = degrees_per_second / 360.0f;
    float motor_revs_per_axis_rev = (float) AXIS_PULLEY_TEETH / (float) MOTOR_PULLEY_TEETH;
    int motor_full_steps = MOTOR_FULL_STEPS_PER_REV;
    uint16_t microsteps = motors_get_microsteps();
    float motor_steps_per_axis_rev = motor_full_steps * (float) microsteps * motor_revs_per_axis_rev;

    motors_state.dec_steps_per_s = axis_rev_per_s * motor_steps_per_axis_rev;
    motors_state.last_update = esp_timer_get_time();
    ESP_LOGI(TAG, "Set axis DEC velocity = %.6f deg/s -> %.1f steps/s (µsteps=%u)",
             degrees_per_second, motors_state.dec_steps_per_s, microsteps);
}
