/* Alpaca bridge — pier side
 *
 * ASCOM convention: 0 = pierEast (telescope on east side of pier,
 * pointing west), 1 = pierWest.  Stored in AlpacaBridgeState — purely
 * informational for clients, the mount does not use it internally.
 */

#include "alpaca_bridge.h"
#include "alpaca_bridge_internal.h"

#include "mount.h"
#include "mount_internal.h"

int alpaca_bridge_get_side_of_pier(void) {
    return alpaca_bridge_state.side_of_pier;
}

MountResult alpaca_bridge_set_side_of_pier(int side) {
    if (side != 0 && side != 1)
        return mount_result_error("Invalid pier side");
    alpaca_bridge_state.side_of_pier = side;
    return mount_result_ok("Pier side set");
}
