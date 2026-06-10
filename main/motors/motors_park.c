/* Motors - motors_park.c
 *
 * Purpose: park both axes immediately.
 *
 * Resets the command queue and sends a single fresh PARK.
 * process_command() sets status = PARKED when the motion task
 * dequeues the PARK.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_park(void) {
    motors_queue_clear();

    MotionCommand cmd = {
        .type = MOTION_CMD_PARK,
        .tracking_mode = TRACKING_NONE,
    };
    motors_queue_send(&cmd);
}
