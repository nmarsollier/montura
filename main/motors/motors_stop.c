/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement via high-priority command.
 *
 * Status is written synchronously so subsequent calls (e.g. motors_home)
 * see READY immediately. The STOP command ensures the motion task
 * also stops its internal loop.
 */
#include "motors.h"
#include "motors_motion.h"

#include "esp_log.h"
#include "esp_timer.h"

void motors_stop(void) {
    /* Soft gate — immediate status transition for callers that follow up. */
    motors_state.status = MOUNT_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;

    /* Hard enforcement — tells the motion task to stop stepping. */
    motors_motion_stop();
}
