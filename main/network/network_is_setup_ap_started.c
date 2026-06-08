/* Network - network_is_setup_ap_started.c
 *
 * Purpose: query whether the setup AP is currently active.
 */
#include "network.h"

bool network_is_setup_ap_started(void) {
    return setup_ap_started;
}
