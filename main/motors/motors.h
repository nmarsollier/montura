#pragma once

#include <stdint.h>

/*
 * High-level state of the motors subsystem used as the authoritative
 * source of truth for mount activity.
 */
typedef enum {
    /* Ready to accept slews/tracking requests. */
    MOUNT_STATUS_READY,
    /* Performing a user-initiated slew or goto. */
    MOUNT_STATUS_SLEWING,
    /* Continuous tracking is active. */
    MOUNT_STATUS_TRACKING,
    /* Parked: mount in safe parked position. */
    MOUNT_STATUS_PARKED,
    /* Disabled: motors unavailable or disabled by user. */
    MOUNT_STATUS_DISABLED
} MotorsStatus;

/* NOTE: `MountStatus` alias removed - use `MotorsStatus` throughout the codebase. */

/*
 * High-level tracking profiles the motors module can apply.
 */
typedef enum TrackingMode {
    /* No automatic tracking. */
    TRACKING_NONE,
    /* Sidereal tracking (stars). */
    TRACKING_SIDEREAL,
    /* Lunar tracking (moon). */
    TRACKING_LUNAR,
    /* Solar tracking (sun). */
    TRACKING_SOLAR
} TrackingMode;

/*
 * Authoritative snapshot of the motors module's view of the mount.
 */
typedef struct {
    /* Current RA axis angle in degrees. */
    float ra_position;
    /* Current DEC axis angle in degrees. */
    float dec_position;
    /* High-level motors-derived status. */
    MotorsStatus status;
    /* Current requested tracking mode. */
    TrackingMode tracking;
    /* Current commanded/measured RA axis angular velocity (deg/s). */
    float ra_velocity;
    /* Current commanded/measured DEC axis angular velocity (deg/s). */
    float dec_velocity;
    /* Current commanded motor step rates derived from velocities (steps/s). */
    float ra_steps_per_s;
    float dec_steps_per_s;

    /* Operational limits enforced by the motors module. */
    struct {
        float ra_min;
        float ra_max;
        float dec_min;
        float dec_max;
    } limits;
} MotorsState;

/* Numeric result codes returned by motors functions. Mount maps these to MountResult objects. */
typedef enum {
    MOTOR_OK = 0,
    MOTOR_ERR_INVALID_AXIS = 1,
    MOTOR_ERR_OUT_OF_RANGE = 2,
    MOTOR_ERR_NOT_READY = 3,
    MOTOR_ERR_INTERNAL = 99
} MotorResultCode;

/*
 * Global state instance owned by the motors module.
 */
extern MotorsState motors_state;

/*
 * Initialize the motors subsystem.
 */
void motors_init(void);

/*
 * Enable or disable motor drivers.
 */
void motors_enable(void);

void motors_disable(void);


/*
 * Return a snapshot copy of the current `MotorsState`.
 */
MotorsState motors_current_state(void);

/*
 * Stop both axes and return to READY.
 */
void motors_stop(void);

void motors_park(void);

/* Convenience high-level home action. */
void motors_home(void);

/* Delegate handling for the STOP button action. */
void motors_button_stop(void);

/*
 * Update the motors module authoritative axis positions.
 */
void motors_sync_position(float ra_axis_deg, float dec_axis_deg);

/*
 * Start continuous tracking according to the chosen `TrackingMode`.
 */
MotorResultCode motors_start_tracking(TrackingMode mode, float lat);

/*
 * Move one or both axes continuously at the given rates in deg/s.
 * Positive = forward, negative = reverse, zero = stop that axis.
 * Both zero is equivalent to STOP.  Used by Alpaca MoveAxis and
 * manual controls (buttons, joystick).
 */
void motors_set_move_axis_velocity(float rate_ra, float rate_dec);

/*
 * Return the RA-axis angular velocity (deg/s) for a given `TrackingMode`.
 */
float motors_get_tracking_speed(TrackingMode mode);

/*
 * Return the module's default slewing speed (deg/s) for a requested profile.
 */
float motors_get_slewing_speed(int speed);

const char *motors_axis_valid_values(void);

/* Canonical status and tracking name helpers — implemented in motors_tools.c. */
const char *status_to_string(MotorsStatus status);

const char *tracking_to_string(TrackingMode tracking);

TrackingMode tracking_from_string(const char *value);

const char *tracking_valid_values(void);

/* Move a single axis to an absolute angle in degrees. */
MotorResultCode motors_slew_axis_to_angle_ra(float degrees, float speed);

MotorResultCode motors_slew_axis_to_angle_dec(float degrees, float speed);

/* Move both axes to absolute angles in degrees. */
MotorResultCode motors_slew_to_angle(float ra_deg, float dec_deg, float speed);

MotorResultCode motors_slew_axis_ra(float degrees, int speed);

MotorResultCode motors_slew_axis_dec(float degrees, int speed);
