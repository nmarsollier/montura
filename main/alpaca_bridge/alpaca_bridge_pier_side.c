/* Alpaca bridge — pier side
 *
 * ASCOM convention: 0 = pierEast, 1 = pierWest.
 *
 * Detection: equatorial_to_axis() in mount_coordinate_tools.c produces
 *   - Normal (no flip):  dec_axis =  DEC + 90°  →  dec_axis ∈ [0°, 180°]
 *   - Flipped (pier flip): dec_axis = -(DEC + 90°)  →  dec_axis ∈ [-180°, 0°)
 *
 * So the sign of the current DEC axis position directly encodes
 * whether the mount flipped — no ambiguous RA-normalisation logic.
 */

#include "alpaca_bridge.h"

#include "mount.h"

int alpaca_bridge_get_side_of_pier(void) {
    MotorsState s = motors_current_state();
    return (s.dec_position >= 0.0f) ? 0   /* pierEast */
                                    : 1;  /* pierWest */
}

MountResult alpaca_bridge_set_side_of_pier(int side) {
    return (MountResult){.ok = true, .message = ""};
}
