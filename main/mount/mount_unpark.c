/* Mount - mount_unpark.c
 *
 * Purpose: return the mount from parked state to normal operation.
 */
#include "mount.h"
#include "mount_internal.h"

#include "esp_log.h"

#include "motors.h"

static const char *TAG = "MOUNT_UNPARK";

/*
 * Business use case: take the mount out of the parked state.
 *
 * Objective: reopen normal operation so pointing, tracking, and sync
 * commands can be accepted again.
 */
MountResult mount_unpark(void) {
    motors_enable();

    ESP_LOGI(TAG, "Mount unparked");

    return mount_result_ok("Mount unparked");
}
