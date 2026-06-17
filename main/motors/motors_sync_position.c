/* Motors - motors_sync_position.c
 *
 * Purpose: update the authoritative axis position immediately.
 * Does not move the motors — only corrects the position model.
 *
 * Applied synchronously so mount_sync returns OK after the update,
 * guaranteeing NINA's subsequent GOTO uses the corrected position.
 *
 * When tracking is active the sync pauses it, applies the correction,
 * and re-enqueues tracking so the mount doesn't silently stop.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_sync_position(float ra_axis_deg, float dec_axis_deg) {
    /* Save tracking state before the sync kills the motion loop. */
    bool was_tracking = (motors_state.status == MOTORS_STATUS_TRACKING
                         && motors_state.tracking != TRACKING_NONE);
    TrackingMode saved_mode = motors_state.tracking;

    motors_motion_sync_apply(ra_axis_deg, dec_axis_deg);

    if (was_tracking) {
        /* Re-enqueue tracking so the motion task resumes from the
         * corrected position.  Same pattern as motors_slew_to_angle. */
        motors_state.status = MOTORS_STATUS_TRACKING;
        motors_state.tracking = saved_mode;

        MotionCommand cmd = {
            .type = MOTION_CMD_TRACK,
            .ra_velocity = motors_state.ra_velocity,
            .dec_velocity = motors_state.dec_velocity,
            .tracking_mode = saved_mode,
        };
        motors_queue_send(&cmd);
    }
}
