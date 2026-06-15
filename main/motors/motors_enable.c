/* Motors - motors_enable.c
 *
 * Purpose: enable motor drivers and bring the subsystem back
 * to an operational state.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_enable(void) {
    motors_hw_enable();
    motors_queue_clear();

    MotionCommand cmd = {
        .type = MOTION_CMD_ENABLE,
        .tracking_mode = TRACKING_NONE,
    };
    motors_queue_send(&cmd);
}
