/* Mount - mount_settings.c
 *
 * Purpose: validate and apply mount settings.
 */
#include "mount.h"
#include "mount_internal.h"

#include "esp_log.h"
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "MOUNT_SETTINGS";

/* mount_set_system_time is implemented in mount_time.c. */

MountResult mount_update_settings(MountSettings settings) {
    if (settings.lat < -90.0f || settings.lat > 90.0f) {
        return mount_result_error("Latitude is outside valid range -90 <= lat <= 90");
    }

    if (settings.lon < -180.0f || settings.lon > 180.0f) {
        return mount_result_error("Longitude is outside valid range -180 <= lon <= 180");
    }

    mount_internal_state = settings;

    /* Persist the updated settings. */
    mount_settings_storage_save(&mount_internal_state);

    ESP_LOGI(
        TAG,
        "Settings updated lat=%.6f lon=%.6f elevation=%d",
        mount_internal_state.lat,
        mount_internal_state.lon,
        mount_internal_state.elevation);

    return mount_result_ok("Settings updated");
}
