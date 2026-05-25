/* Mount - mount_stop.c
 *
 * Purpose: stop any active mount movement.
 */
#include "mount.h"
#include "mount_internal.h"

#include "esp_log.h"

#include "motors.h"

static const char *TAG = "MOUNT_STOP";

/*
 * Business use case: stop an active movement operation.
 *
 * Objective: leave the mount ready for the next command after a STOP request.
 */
MountResult mount_stop(void) {
    motors_stop();

    ESP_LOGI(TAG, "Motion stopped");

    return mount_result_ok("Motion stopped");
}
