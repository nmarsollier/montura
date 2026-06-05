#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Property — DestinationSideOfPier
 *
 * Purpose: Not implemented. Returns error 1024.
 *
 * Alpaca usage: N.I.N.A. may call this before a meridian flip; NotImplemented is handled gracefully.
 */
esp_err_t alpaca_destinationsideofpier_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_error(req, 1024,
                          "DestinationSideOfPier not implemented", cid, stx);
    return ESP_OK;
}
