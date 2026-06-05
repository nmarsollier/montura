#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Capability — CanSyncAltAz
 *
 * Purpose: Returns false — Alt/Az sync not supported (EQ mount).
 *
 * Alpaca usage: N.I.N.A. uses equatorial sync instead.
 */
esp_err_t alpaca_cansyncaltaz_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = false;
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
