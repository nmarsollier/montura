/* Motors - motors_park.c
 *
 * Purpose: park both axes via high-priority command.
 */
#include "motors.h"
#include "motors_motion.h"

void motors_park(void) {
    motors_state.status = MOUNT_STATUS_PARKED;
    motors_state.tracking = TRACKING_NONE;

    motors_motion_park();
}
