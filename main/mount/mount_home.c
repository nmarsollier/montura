/* Mount - mount_home.c
 *
 * Purpose: send the mount to its home position.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

/*
 * Business use case: send the mount to its home position.
 *
 * Objective: move the mount to the home position and stop the motors first.
 */
MountResult mount_home(void) {
    if (motors_current_state().status == MOUNT_STATUS_PARKED) {
        return mount_result_ok("Mount parked");
    }

    motors_home();

    return mount_result_ok("Mount homed");
}
