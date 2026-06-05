#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"

/* Alpaca — Method — SyncToTarget
 *
 * Purpose: Syncs the mount to the previously set target coordinates.
 *
 * Alpaca usage: N.I.N.A. sets target first, then calls this after a plate solve.
 */
esp_err_t alpaca_synctotarget_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    MountResult response = mount_alpaca_sync_to_target();
    if (response.ok)
        alpaca_response_ok(req, cid, stx);
    else
        alpaca_response_error(req, 1025, response.message, cid, stx);
    return ESP_OK;
}
