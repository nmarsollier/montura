#pragma once

/* Internal shared state for the motors_motion module. */

extern volatile float motors_motion_target_ra_deg;
extern volatile float motors_motion_target_dec_deg;

/* Start positions captured when motion begins — used for acceleration ramps. */
extern volatile float motors_motion_start_ra_deg;
extern volatile float motors_motion_start_dec_deg;
