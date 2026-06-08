/* REST - rest_sync_handler.c
 *
 * Purpose: handle mount synchronization requests.
 */
#include "rest.h"

#include <stdio.h>

#include "mount.h"
#include "tools/tools.h"

/*
 * Business use case: expose SYNC via the API for remote recalibration.
 *
 * Objective: let clients adjust the pointing reference when a reliable field
 * position is available.
 */
esp_err_t rest_sync_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonFloatResult ra = json_get_float(body.value, "ra");
    JsonFloatResult dec = json_get_float(body.value, "dec");

    if (!ra.ok) {
        http_response_bad_request(request, "Missing or invalid 'ra'. Valid values: [ra:0<=ra<24]");
        return ESP_OK;
    }

    if (!dec.ok) {
        http_response_bad_request(request, "Missing or invalid 'dec'. Valid values: [dec:-90<=dec<=90]");
        return ESP_OK;
    }

    rest_send_result(request, mount_sync(ra.value, dec.value));

    return ESP_OK;
}
