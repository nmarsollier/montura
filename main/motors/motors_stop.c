/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement immediately.
 *
 * Resets the command queue and sends a single fresh STOP.
 * process_command() in the motion task sets status = READY
 * when it dequeues the STOP.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_stop(void) {
    motors_queue_clear();

    MotionCommand cmd = {
        .type = MOTION_CMD_STOP,
        .tracking_mode = TRACKING_NONE,
    };
    motors_queue_send(&cmd);
}
