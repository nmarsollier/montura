/* Motors - motors_start_tracking.c
 *
 * Purpose: start continuous tracking for the selected mode.
 *
 * Sends a high-priority TRACK command that will preempt any
 * in-progress slew, giving tracking absolute priority.
 *
 * Status is written synchronously so external readers immediately
 * see the tracking state. The command queue handles preemption.
 */
#include "motors.h"
#include "motors_motion.h"
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_START_TRACKING";

MotorResultCode motors_start_tracking(TrackingMode mode, float lat) {
    float ra_speed = motors_get_tracking_speed(mode);

    /* Southern hemisphere: tracking direction is reversed. */
    if (lat < 0.0f) ra_speed = -ra_speed;

    /* Pre-configure velocities for the task to pick up. */
    motors_set_axis_velocity_ra(ra_speed);
    motors_set_axis_velocity_dec(0.0f);

    /* Soft gate — visible to status readers immediately. */
    motors_state.tracking = mode;
    motors_state.status   = MOUNT_STATUS_TRACKING;

    /*
     * High-priority command — preempts any SLEW in progress.
     * No explicit stop needed; the task handles preemption.
     */
    motors_motion_track(mode, ra_speed);

    ESP_LOGI(TAG, "Tracking started: mode=%s RA_speed=%.6f", tracking_to_string(mode), ra_speed);
    return MOTOR_OK;
}
