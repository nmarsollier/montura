#pragma once

#include "motors.h"
#include <stdbool.h>

/*
 * Update the commanded velocity for a physical axis and derive step rate.
 * For internal use only; called before starting motion.
 */
void motors_set_axis_velocity_ra(float degrees_per_second);

void motors_set_axis_velocity_dec(float degrees_per_second);

/* Validate axis values against the configured inclusive limits. */
bool motors_is_valid_ra(float value);

bool motors_is_valid_dec(float value);

/* --------------------------------------------------------------------------
 * Mechanical constants — hardware configuration
 * -------------------------------------------------------------------------- */
#define MOTOR_STEP_ANGLE_DEG     (1.8f)
#define MOTOR_FULL_STEPS_PER_REV ((int)(360.0f / MOTOR_STEP_ANGLE_DEG))
#define MOTOR_PULLEY_TEETH       (20)
#define AXIS_PULLEY_TEETH        (80)

/*
 * Motion calibration factor.
 *
 * Compensates for discrepancies between configured and actual step resolution.
 * When commanding 180° results in 90° physical movement, the factor is 2.0.
 *
 * Adjust this value until commanded angles match physical movement:
 *   - Mount moves too little → increase the factor
 *   - Mount moves too much   → decrease the factor
 *
 * Formula: factor = commanded_angle / actual_angle
 * Example:  2.0 = 180° / 90°
 */
#define MOTION_CALIBRATION_FACTOR 1.0f

/* --------------------------------------------------------------------------
 * Microstep and step resolution — sourced from TMC hardware
 *
 * The TMC2209 driver is the SINGLE source of truth for microstep count.
 * Use the inline helper below to always read the active value from the
 * TMC module's verified cache (no UART transaction needed on read).
 *
 * Fallback: if TMC not yet initialized, returns TMC_TARGET_MICROSTEPS (128).
 * -------------------------------------------------------------------------- */

/*
 * Get the active microstep count from the TMC2209 driver.
 * This reads the cached value that was verified against hardware registers
 * during tmc2209_hw_init().
 *
 * @return Microstep count (e.g. 128), or 128 as fallback if TMC not ready.
 */
static inline uint16_t motors_get_microsteps(void) {
    extern uint16_t tmc2209_get_active_microsteps(void);
    uint16_t ms = tmc2209_get_active_microsteps();
    return (ms > 0) ? ms : 128;
}

/*
 * Angular displacement per microstep at the mount axis.
 * Computed at runtime using the TMC-verified microstep count.
 *
 * Formula: 360° / (200 full-steps × microsteps × (80/20) × calibration)
 *
 * With 128 µsteps and calibration 1.0: 360 / 102,400 = 0.003515625°
 */
static inline float motors_get_deg_per_microstep(void) {
    return 360.0f / ((float) MOTOR_FULL_STEPS_PER_REV *
                     (float) motors_get_microsteps() *
                     ((float) AXIS_PULLEY_TEETH / (float) MOTOR_PULLEY_TEETH) *
                     MOTION_CALIBRATION_FACTOR);
}
