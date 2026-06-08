/* Motors - motors_move_axis_velocity.c
 *
 * Purpose: request continuous axis velocity motion.
 * Delegates to the motors_motion command queue.
 */
#include "motors.h"
#include "motors_motion.h"

void motors_set_move_axis_velocity(float rate_ra, float rate_dec) {
    motors_motion_move_axis(rate_ra, rate_dec);
}
