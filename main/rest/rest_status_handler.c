#include "rest.h"

#include <stdio.h>
#include <time.h>

#include "mount.h"

#include "utils/utils.h"
#include "network/network.h"

/*
 * Business use case: expose the mount's current status via the API.
 *
 * Objective: provide a consistent operational snapshot for UI, monitoring,
 * and external automation.
 */
esp_err_t rest_status_handler(httpd_req_t *request) {
    VisibleStatusData data = mount_get_visible_status();

    /* Format current mount time as ISO 8601 for the UI. */
    time_t now = time(NULL);
    struct tm tm = {0};
    gmtime_r(&now, &tm);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", &tm);

    bool is_home = (data.status == MOUNT_STATUS_READY
                    && data.ra.hours == 0 && data.ra.minutes == 0
                    && data.dec.degrees == 0 && data.dec.minutes == 0);

    static const char format[] =
            "{"
            "\"status\":\"%s\","
            "\"tracking\":\"%s\","
            "\"ra\":\"%02d:%02d:%05.2f\","
            "\"dec\":\"%c%02d:%02d:%05.2f\","
            "\"time\":\"%s\","
            "\"settings\":{"
            "\"lat\":%.6f,"
            "\"lon\":%.6f,"
            "\"elevation\":%d"
            "},"
            "\"wifi_ap\":%s,"
            "\"is_home\":%s"
            "}";

    const char *status = status_to_string(data.status);
    const char *tracking = tracking_to_string(data.tracking);
    char dec_sign = data.dec.sign >= 0 ? '+' : '-';
    bool wifi_ap = network_is_setup_ap_started();

    /*
     * Fixed-size buffer — the JSON response fits comfortably in 512 bytes.
     * Avoiding snprintf(NULL, 0, ...) + VLA removes the duplicate format call.
     */
    char response[512];
    snprintf(response, sizeof(response), format,
             status, tracking,
             data.ra.hours, data.ra.minutes, data.ra.seconds,
             dec_sign, data.dec.degrees, data.dec.minutes, data.dec.seconds,
             time_buf,
             data.settings.lat, data.settings.lon, data.settings.elevation,
             wifi_ap ? "true" : "false",
             is_home ? "true" : "false");

    http_response_json(request, response);

    return ESP_OK;
}
