#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"
#include <stdio.h>

/* Alpaca — Property — UTCDate (GET)
 *
 * Purpose: Returns the current UTC date as an ISO 8601 string.
 *
 * Alpaca usage: N.I.N.A. reads this to verify time sync between mount and PC.
 */
esp_err_t alpaca_utcdate_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();

    const char *response = mount_alpaca_get_utc_date();

    char buf[64];
    snprintf(buf, sizeof(buf), "\"%s\"", response);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
