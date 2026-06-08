/* Motors - motors_enable.c
 *
 * Purpose: enable motor movement via command queue.
 */
#include "motors.h"
#include "motors_motion.h"

#include "esp_log.h"
#include "esp_timer.h"

/*
 * Bring the motors back to an operational state.
 * Hardware enable happens immediately; the command notifies the task.
 */
void motors_enable(void) {
    motors_motion_hw_enable();

    motors_state.status = MOUNT_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;

    motors_motion_enable();
}
