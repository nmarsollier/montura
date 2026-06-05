#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"
#include <math.h>

/* Alpaca — Method — MoveAxis
 *
 * Purpose: Moves a single axis (0 = RA, 1 = DEC) at the specified rate in deg/s.
 *
 * Alpaca usage: Used by N.I.N.A. for manual joystick-style motion controls.
 */
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

    MotorAxis ma = (axis == 0) ? MOTOR_AXIS_RA : MOTOR_AXIS_DEC;
    float degrees = rate * 0.1f;
    MountResult result = mount_move_axis(ma, degrees, (int) fabsf(rate));
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
