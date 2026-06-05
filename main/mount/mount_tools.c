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

/*
 * Convert RA from HMS struct to decimal hours.
 * Used by the Alpaca layer and screen module to report the current position.
 */
float mount_get_ra_hours(void) {
    VisibleStatusData d = mount_get_visible_status_data();
    return (float) d.ra.hours + (float) d.ra.minutes / 60.0f
           + d.ra.seconds / 3600.0f;
}

/*
 * Convert DEC from DMS struct to decimal degrees.
 */
float mount_get_dec_deg(void) {
    VisibleStatusData d = mount_get_visible_status_data();
    float deg = (float) d.dec.degrees + (float) d.dec.minutes / 60.0f
                + d.dec.seconds / 3600.0f;
    return (float) d.dec.sign * deg;
}

MountResult mount_result_error(const char *message) {
    MountResult result = {
        .ok = false,
        .message = message
    };

    ESP_LOGW(TAG, "Rejected command: %s", message);

    return result;
}
