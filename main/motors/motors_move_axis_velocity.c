/* Motors - motors_move_axis_velocity.c
 *
 * Purpose: request continuous single-axis velocity motion.
 * Positive rate = forward, negative = reverse, zero = stop that axis.
 * Used by Alpaca MoveAxis, physical buttons, joystick, and guiding.
 */
#include "motors.h"
#include "motors_internal.h"

#include <math.h>

void motors_set_move_axis_velocity(float rate_ra, float rate_dec) {
    if (fabsf(rate_ra) < 1e-9f && fabsf(rate_dec) < 1e-9f) {
        motors_stop();
        return;
    }

    MotionCommand cmd = {
        .type = MOTION_CMD_MOVE_AXIS,
        .ra_target_deg = 0.0f, /* set by the task from limits */
        .dec_target_deg = 0.0f,
        .ra_velocity = rate_ra,
        .dec_velocity = rate_dec,
        .tracking_mode = TRACKING_NONE,
    };
    motors_queue_send(&cmd);
}
