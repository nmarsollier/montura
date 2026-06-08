/* Motors - motors_sync_position.c
 *
 * Purpose: update the authoritative axis position model via command queue.
 */
#include "motors.h"
#include "motors_motion.h"

void motors_sync_position(float ra_axis_deg, float dec_axis_deg) {
    motors_motion_sync(ra_axis_deg, dec_axis_deg);
}
