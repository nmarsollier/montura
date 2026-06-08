/* MotorsMotion - motors_motion_slew_relative.c
 *
 * Purpose: enqueue a relative slew (delta degrees) for one or both axes.
 * The motion task computes the absolute target from the live axis position
 * at dequeue time, so queued commands stack correctly.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

void motors_motion_slew_relative(float ra_delta, float dec_delta,
                                 float ra_vel, float dec_vel) {
    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_target_deg = 0.0f,   /* computed at dequeue time */
        .dec_target_deg = 0.0f,
        .ra_velocity = ra_vel,
        .dec_velocity = dec_vel,
        .tracking_mode = TRACKING_NONE,
        .relative = true,
        .ra_delta_deg = ra_delta,
        .dec_delta_deg = dec_delta,
    };
    motors_motion_cmd_send(&cmd, true);  /* high priority — preempts TRACK */
}
