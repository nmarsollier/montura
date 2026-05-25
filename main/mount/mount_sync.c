/* Mount - mount_sync.c
 *
 * Purpose: synchronize the pointing model with a known sky position.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

#include "esp_log.h"

static const char *TAG = "MOUNT_SYNC";

/*
 * Business use case: synchronize the mount's pointing model.
 *
 * Objective: recalibrate the internal reference with known coordinates so
 * future GOTO and tracking operations stay accurate.
 */
MountResult mount_sync(float ra, float dec) {
    if (ra < 0.0f || ra >= 24.0f) {
        return mount_result_error("RA is outside valid range 0 <= ra < 24");
    }

    if (dec < -90.0f || dec > 90.0f) {
        return mount_result_error("DEC is outside valid range -90 <= dec <= 90");
    }

    /* SYNC does not move the physical axes; it only updates the authoritative
   * position model to match the known sky position. */

    EquatorialCoordinates requested = {.ra_hours = ra, .dec_deg = dec};
    AxisCoordinates axis = equatorial_to_axis(requested);

    motors_sync_position(axis.ra_axis_deg, axis.dec_axis_deg);

    ESP_LOGI(TAG, "SYNC applied RA=%.3f DEC=%.3f -> AXIS RAdeg=%.3f DECdeg=%.3f",
             ra, dec, axis.ra_axis_deg, axis.dec_axis_deg);

    return mount_result_ok("SYNC applied");
}
