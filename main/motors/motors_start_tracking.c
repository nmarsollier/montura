/* Motors - motors_start_tracking.c
 *
 * Purpose: start continuous tracking for the selected mode.
 */
#include "motors.h"
#include "motors_motion.h"
#include "motors_internal.h"

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MOTORS_START_TRACKING";

MotorResultCode motors_start_tracking(TrackingMode mode) {
    motors_stop();

    float ra_speed = motors_get_tracking_speed(mode);

    motors_set_axis_velocity_ra(ra_speed);
    motors_set_axis_velocity_dec(0.0f);

    motors_state.tracking = mode;
    motors_state.status = MOUNT_STATUS_TRACKING;

    /*
     * For tracking, the RA target is set to the axis limit so the task runs
     * open-ended. The task will not stop on its own while status is TRACKING —
     * it only terminates when status changes externally (stop, park, etc.).
     */
    motors_motion_start(motors_state.limits.ra_max, motors_state.dec_position);

    ESP_LOGI(TAG, "Tracking started: mode=%d RA_speed=%.6f", mode, ra_speed);
    return MOTOR_OK;
}
