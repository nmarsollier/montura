/* Motors - motors_sync_position.c
 *
 * Purpose: update the authoritative axis position immediately.
 * Does not move the motors — only corrects the position model.
 *
 * Applied synchronously so mount_sync returns OK after the update,
 * guaranteeing NINA's subsequent GOTO uses the corrected position.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_sync_position(float ra_axis_deg, float dec_axis_deg) {
    motors_motion_sync_apply(ra_axis_deg, dec_axis_deg);
}
