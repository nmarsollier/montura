/* Mount - mount_tools.c
 *
 * Purpose: mount result helpers and command validation support.
 */
#include "mount.h"
#include "mount_internal.h"

#include "esp_log.h"

#include "motors.h"

static const char *TAG = "MOUNT_TOOLS";

/*
 * Business use case: normalize mount command responses.
 *
 * Objective: provide a uniform success/error contract for REST and UI
 * clients.
 */
MountResult mount_result_ok(const char *message) {
    MountResult result = {
        .ok = true,
        .message = message
    };

    return result;
}

MountResult mount_result_error(const char *message) {
    MountResult result = {
        .ok = false,
        .message = message
    };

    ESP_LOGW(TAG, "Rejected command: %s", message);

    return result;
}
