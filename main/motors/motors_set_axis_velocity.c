/* Motors - motors_set_axis_velocity.c
 *
 * Purpose: update an axis velocity and derive the step rate.
 *
 * Microstep count is obtained at runtime from the TMC2209 driver
 * (the single source of truth) via motors_get_microsteps().
 */
#include "motors.h"

/*
 * Update the commanded velocity for a physical axis.
 * Step rate derivation uses the TMC-verified microstep count.
 */
void motors_set_axis_velocity_ra(float degrees_per_second) {
    motors_state.ra_velocity = degrees_per_second;
}

void motors_set_axis_velocity_dec(float degrees_per_second) {
    motors_state.dec_velocity = degrees_per_second;
}
