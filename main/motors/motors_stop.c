/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement via high-priority command.
 *
 * Status is written synchronously so subsequent calls (e.g. motors_home)
 * see READY immediately. The STOP command ensures the motion task
 * also stops its internal loop.  Any queued motion commands are drained
 * so a stale TRACK / SLEW doesn't restart motion after the stop.
 */
#include "motors.h"
#include "motors_motion.h"
#include "motors_motion_internal.h"

void motors_stop(void) {
    /* Soft gate — immediate status transition for callers that follow up. */
    motors_state.status = MOUNT_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;

    /* Hard enforcement — tells the motion task to stop stepping. */
    motors_motion_stop();

    /*
     * Drain any queued TRACK / SLEW / MOVE_AXIS commands that were sent
     * before the STOP.  Without this the motion task would pick them up
     * after processing the STOP and immediately restart motion.
     */
    if (motion_cmd_queue != NULL) {
        MotionCommand discard;
        while (xQueueReceive(motion_cmd_queue, &discard, 0) == pdTRUE) {
            /* Keep only STOP / PARK / DISABLE — discard motion commands. */
            if (discard.type == MOTION_CMD_STOP ||
                discard.type == MOTION_CMD_PARK ||
                discard.type == MOTION_CMD_DISABLE) {
                xQueueSendToBack(motion_cmd_queue, &discard, 0);
            }
        }
    }
}
