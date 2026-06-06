/* Mount - mount_tracking.c
 *
 * Purpose: change the active tracking mode.
 */
#include "mount.h"
#include "mount_internal.h"

#include "esp_log.h"

#include "motors.h"

static const char *TAG = "MOUNT_TRACKING";

/*
 * Business use case: change the mount's active tracking mode.
 *
 * Objective: let clients choose how the mount compensates for the sky's
 * apparent motion during observations while respecting state rules.
 */
MountResult mount_set_tracking(TrackingMode tracking) {
    MotorResultCode rc = motors_start_tracking(tracking);
    if (rc != MOTOR_OK) {
        switch (rc) {
            case MOTOR_ERR_NOT_READY:
                return mount_result_error("Motors not ready");
            default:
                return mount_result_error("Motor error");
        }
    }

    ESP_LOGI(TAG, "Tracking changed to %s", tracking_to_string(tracking));

    return mount_result_ok("Tracking changed");
}
