/* Motors - motors_disable.c
 *
 * Purpose: disable motor drivers immediately.
 *
 * Kills hardware and resets the queue, then sends a single
 * fresh DISABLE so the motion task aligns its internal state.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_disable(void) {
    motors_queue_clear();

    /* Kill hardware immediately — don't wait for the motion task. */
    motors_hw_disable();

    MotionCommand cmd = {
        .type = MOTION_CMD_DISABLE,
        .tracking_mode = TRACKING_NONE,
    };
    motors_queue_send(&cmd);
}
