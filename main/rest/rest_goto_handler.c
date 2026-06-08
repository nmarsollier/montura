/* REST - rest_goto_handler.c
 *
 * Purpose: handle GOTO requests.
 */
#include "rest.h"

#include <stdio.h>
#include <string.h>

#include "mount.h"

#include "tools/tools.h"

/*
 * Business use case: expose GOTO via the API with input validation.
 *
 * Objective: accept target coordinates from external clients and convert them
 * into a safe movement request.
 */
esp_err_t rest_goto_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonFloatResult ra = json_get_float(body.value, "ra");
    JsonFloatResult dec = json_get_float(body.value, "dec");
    JsonIntResult speed = json_get_int(body.value, "speed");
    int speed_value = 1;

    if (!ra.ok) {
        http_response_bad_request(request, "Missing or invalid 'ra'. Valid values: [0<=ra<24]");
        return ESP_OK;
    }

    if (!dec.ok) {
        http_response_bad_request(request, "Missing or invalid 'dec'. Valid values: [-90<=dec<=90]");
        return ESP_OK;
    }

    bool has_speed = strstr(body.value, "\"speed\"") != NULL;

    if (has_speed && !speed.ok) {
        http_response_bad_request(request, "Invalid 'speed'. Valid values: [1<=speed<=4]");
        return ESP_OK;
    }

    if (has_speed) {
        speed_value = speed.value;
    }

    rest_send_result(request, mount_goto(ra.value, dec.value, speed_value));

    return ESP_OK;
}
