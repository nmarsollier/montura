#include "rest.h"

#include <stdio.h>
#include <string.h>

#include "tools/tools.h"

/*
 * Business use case: unify REST command responses.
 *
 * Objective: keep the `ok` + `message` contract consistent and map failures
 * to HTTP 409 for clients.
 */
void rest_send_result(httpd_req_t *request, MountResult result) {
    static const char format[] = "{\"ok\":%s,\"message\":\"%s\"}";
    char response[strlen(result.message) + sizeof(format) + 1];

    snprintf(
        response,
        sizeof(response),
        format,
        result.ok ? "true" : "false",
        result.message);

    if (!result.ok) {
        httpd_resp_set_status(request, "409 Conflict");
    }

    http_response_json(request, response);
}
