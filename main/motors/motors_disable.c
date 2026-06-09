/* Motors - motors_disable.c
 *
 * Purpose: disable motor movement via high-priority command.
 */
#include "motors.h"
#include "motors_motion.h"

/*
 * Place the motors subsystem into a disabled state.
 */
void motors_disable(void) {
    motors_state.status = MOUNT_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
    motors_motion_stop();

    motors_motion_hw_disable();

    motors_state.status = MOUNT_STATUS_DISABLED;

    motors_motion_disable();
}
