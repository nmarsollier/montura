#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Property — AxisRates
 *
 * Purpose: Returns step rate ranges for RA and DEC axes (0.004 to 32 deg/s).
 *
 * Alpaca usage: N.I.N.A. uses this for MoveAxis speed limits.
 */
esp_err_t alpaca_axisrates_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_value(req,
                          "[{\"Minimum\":0.004178,\"Maximum\":32.0},"
                          "{\"Minimum\":0.004178,\"Maximum\":32.0}]",
                          cid, stx);
    return ESP_OK;
}
