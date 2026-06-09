#include "mount.h"
#include "mount_internal.h"
#include "motors.h"

/*
 * Business use case: move one or both axes continuously at the given rates.
 *
 * Objective: support Alpaca MoveAxis (joystick-style controls) where the
 * client sends a rate and expects continuous motion until rate = 0.
 */
MountResult mount_set_move_axis_velocity(float rate_ra, float rate_dec) {
    motors_set_move_axis_velocity(rate_ra, rate_dec);

    return mount_result_ok();
}
