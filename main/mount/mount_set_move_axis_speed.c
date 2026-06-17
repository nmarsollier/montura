#include "mount.h"
#include "mount_internal.h"
#include "motors.h"

/*
 * Business use case: move one or both axes continuously at the given rates.
 *
 * Objective: support Alpaca MoveAxis (joystick-style controls) where the
 * client sends a rate and expects continuous motion until rate = 0.
 */
MountResult mount_set_move_axis_speed(float ra_speed, float dec_speed) {
    if ((int) ra_speed == 0 && (int) dec_speed == 0) {
        return mount_stop();
    }

    /* Stop tracking before sending the move command — the tracking loop
     * ignores non-terminal commands and a follow-up stop would clear the
     * queue, discarding the unprocessed MOVE_AXIS. */
    MotorsState s = motors_current_state();
    if (s.status == MOTORS_STATUS_TRACKING && s.tracking != TRACKING_NONE) {
        mount_stop();
    }

    motors_set_move_axis_speed(ra_speed, dec_speed);

    return mount_result_ok();
}
