#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Shared logic for handlers that parse RA/Dec form params and delegate
 * to a bridge action.  Used by SlewToCoordinates and SyncToCoordinates. */
static esp_err_t handle_coordinates(httpd_req_t *req,
                                    MountResult (*action)(float, float)) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float ra = 0.0f, dec = 0.0f;

    if (!alpaca_get_form_float(req, "RightAscension", &ra) ||
        !alpaca_get_form_float(req, "Declination", &dec)) {
        alpaca_response_error(req, 1025, "Missing RightAscension or Declination",
                              cid, stx);
        return ESP_OK;
    }

    MountResult r = action(ra, dec);
    if (r.ok)
        alpaca_response_ok(req, cid, stx);
    else
        alpaca_response_error(req, 1025, r.message, cid, stx);
    return ESP_OK;
}

/* Alpaca — Method — SlewToCoordinatesAsync
 *
 * Purpose: Starts an asynchronous slew to the given RA (hours) and DEC (degrees).
 *
 * Alpaca usage: Primary slew method used by N.I.N.A. for all automated slews.
 */
esp_err_t alpaca_slewtocoordinatesasync_handler(httpd_req_t *req) {
    return handle_coordinates(req, alpaca_bridge_slew_to_coordinates);
}

/* Alpaca — Method — SyncToCoordinates
 *
 * Purpose: Syncs the mount's internal position to the given coordinates without moving.
 *
 * Alpaca usage: Called by N.I.N.A. after plate-solving to correct pointing.
 */
esp_err_t alpaca_synctocoordinates_handler(httpd_req_t *req) {
    return handle_coordinates(req, alpaca_bridge_sync_to_coordinates);
}
