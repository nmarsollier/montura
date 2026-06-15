/* Alpaca bridge — destination pier side
 *
 * Computes the pier side that would result from a slew to the given
 * equatorial coordinates at the current time and site.
 *
 * Uses the same equatorial_to_axis() logic as mount_goto():
 *   - Normal (no flip):  dec_axis >= 0  →  pierEast  (0)
 *   - Flipped:           dec_axis <  0  →  pierWest  (1)
 */

#include "alpaca_bridge.h"

#include "mount.h"

int alpaca_bridge_get_destination_side_of_pier(float ra_hours, float dec_deg) {
    EquatorialCoordinates eq = {
        .ra_hours = ra_hours,
        .dec_deg = dec_deg
    };

    AxisCoordinates axis = equatorial_to_axis(eq);

    return (axis.dec_axis_deg >= 0.0f) ? 0   /* pierEast */
                                       : 1;  /* pierWest */
}
