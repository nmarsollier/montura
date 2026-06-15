/* Mount - mount_sync_position.c
 *
 * Purpose: set the mount's axis position directly (no physical move).
 * Thin wrapper around motors_sync_position.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

MountResult mount_sync_position(float ra_axis_deg, float dec_axis_deg) {
    motors_sync_position(ra_axis_deg, dec_axis_deg);
    return mount_result_ok();
}
