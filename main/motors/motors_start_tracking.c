/* Motors - motors_start_tracking.c
 *
 * Purpose: start continuous tracking for the selected mode.
 *
 * Status and tracking mode are set by process_command() when the
 * motion task dequeues the TRACK command — no soft gate needed.
 */
#include "motors.h"
#include "motors_internal.h"

MotorResultCode motors_start_tracking(TrackingMode mode, float lat) {
    if (mode == TRACKING_NONE) {
        motors_stop();
        return MOTOR_OK;
    }

    float ra_speed = motors_get_tracking_speed(mode);

    /* Southern hemisphere: tracking direction is reversed. */
    if (lat < 0.0f) ra_speed = -ra_speed;

    motors_state.ra_speed = ra_speed;
    motors_state.dec_speed = 0.0f;

    MotionCommand cmd = {
        .type = MOTION_CMD_TRACK,
        .ra_target_deg = 0.0f,   /* set by the task from limits */
        .dec_target_deg = 0.0f,
        .ra_speed = ra_speed,
        .dec_speed = 0.0f,
        .tracking_mode = mode,
    };
    motors_queue_put(&cmd);

    return MOTOR_OK;
}
