#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Method — SyncToAltAz
 *
 * Purpose: Not implemented — equatorial mount cannot sync in Alt/Az.
 *
 * Alpaca usage: Returns NotImplemented error (1024).
 */
esp_err_t alpaca_synctoaltaz_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_error(req, 1024, "Alt/Az sync not implemented",
                          cid, stx);
    return ESP_OK;
}
