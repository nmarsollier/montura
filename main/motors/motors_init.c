/* Motors - motors_init.c
 *
 * Purpose: initialize the motors module — state, hardware, queue, and motion task.
 */
#include "motors.h"
#include "motors_internal.h"

/*
 * Default motors state — positions in degrees, home = alignment reference.
 * Axis limits are configured via .limits (see MotorsState).
 */
MotorsState motors_state = {
    .ra_position = 0.0f,
    .dec_position = 0.0f,
    .status = MOTORS_STATUS_READY,
    .tracking = TRACKING_NONE,
    .ra_velocity = 0.0f,
    .dec_velocity = 0.0f,
    .limits = {
        .ra_min = -100.0f,
        .ra_max = 100.0f,
        .dec_min = -150.0f,
        .dec_max = 150.0f,
    },
};

void motors_init(void) {
    motors_hw_init();
    motors_queue_init();
    motors_motion_task_init();
}
