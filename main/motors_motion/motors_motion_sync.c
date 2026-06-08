/* MotorsMotion - motors_motion_sync.c
 *
 * Purpose: align the internal position model to the given axis angles
 * via the command queue for thread-safe execution.
 * Does not move the motors — only updates the authoritative position.
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"

void motors_motion_sync(float ra_axis_deg, float dec_axis_deg) {
    MotionCommand cmd = {
        .type = MOTION_CMD_SYNC,
        .ra_target_deg = ra_axis_deg,
        .dec_target_deg = dec_axis_deg,
        .tracking_mode = TRACKING_NONE,
    };
    motors_motion_cmd_send(&cmd, false);
}
