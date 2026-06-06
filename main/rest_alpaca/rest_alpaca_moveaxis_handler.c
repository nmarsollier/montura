/* Alpaca — Method — MoveAxis
 *
 * Purpose: moves a single axis (0 = RA, 1 = DEC) continuously at the
 * given rate in deg/s until a subsequent call with Rate = 0 stops it.
 *
 * Alpaca / ASCOM semantics: MoveAxis does NOT perform a relative move.
 * It starts (or changes) continuous axis motion.  The client (e.g. N.I.N.A.)
 * calls MoveAxis periodically while a joystick button is held, then calls
 * MoveAxis(axis, 0) on release.
 */
#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"
#include <math.h>

/* Remember the last rate for each axis so setting one axis to 0
 * does not inadvertently stop the other. */
static float s_ra_rate  = 0.0f;
static float s_dec_rate = 0.0f;

esp_err_t alpaca_moveaxis_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int axis = 0;
    float rate = 0.0f;

    if (!alpaca_get_form_int(req, "Axis", &axis) ||
        !alpaca_get_form_float(req, "Rate", &rate)) {
        alpaca_response_error(req, 1025, "Missing Axis or Rate", cid, stx);
        return ESP_OK;
    }

    /* Update the rate for the requested axis, keep the other axis unchanged. */
    if (axis == 0) {
        s_ra_rate = rate;
    } else {
        s_dec_rate = rate;
    }

    MountResult result = mount_move_axis_velocity(s_ra_rate, s_dec_rate);
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
