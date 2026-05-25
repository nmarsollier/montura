/* Mount - mount_park.c
 *
 * Purpose: park the mount safely.
 */
#include "mount.h"
#include "mount_internal.h"

#include "esp_log.h"

#include "motors.h"

static const char *TAG = "MOUNT_PARK";

/*
 * Business use case: park the mount.
 *
 * Objective: leave the equipment in a safe rest state with tracking disabled.
 */
MountResult mount_park(void) {
    if (motors_current_state().status == MOUNT_STATUS_PARKED) {
        return mount_result_ok("Mount parked");
    }

    motors_park();

    ESP_LOGI(TAG, "Mount parked");

    return mount_result_ok("Mount parked");
}
