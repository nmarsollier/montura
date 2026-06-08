/* Motors - motors_init.c
 *
 * Purpose: initialize the motors module and its default state.
 */
#include "motors.h"

#include "motors_motion.h"

/*
 * Default motors state.
 *
 * Positions are expressed in degrees.
 *   RA  -120..+120  (home = meridian; physical tripod collision risk beyond)
 *   DEC -180..+180  (home = south celestial pole; cable wrap prevention)
 * The initial value is 0.0, which represents the home/alignment reference.
 */
MotorsState motors_state = {
    .ra_position = 0.0f,
    .dec_position = 0.0f,
    .status = MOUNT_STATUS_READY,
    .tracking = TRACKING_NONE,
    .ra_velocity = 0.0f,
    .dec_velocity = 0.0f,
    .limits = {
        .ra_min = -120.0f,
        .ra_max = 120.0f,
        .dec_min = -180.0f,
        .dec_max = 180.0f,
    },
};

void motors_init(void) {
    /* Keep this entry point for future hardware initialization. */
    motors_motion_init();
}
