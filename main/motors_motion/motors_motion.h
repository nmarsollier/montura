#pragma once

#include "motors.h"

#include <stdint.h>

#include "esp_err.h"

/*
 * motors_motion — hardware motion execution layer.
 *
 * Owns the FreeRTOS motion task and all step-level execution.
 * Receives commands from the motors module via a priority-aware queue
 * and drives axis positions autonomously until the requested motion
 * is complete or preempted by a higher-priority command.
 *
 * The motion task is the single writer of motors_state position fields,
 * guaranteeing thread-safe state transitions without locks.
 */

/*
 * Initialize the motion subsystem, create the command queue, and
 * start the background motion task.
 * Must be called once from motors_init() before any motion is requested.
 */
void motors_motion_init(void);

/* --------------------------------------------------------------------------
 * Command queue API — thread-safe, priority-aware.
 *
 * These functions are the canonical way to request motion from the
 * motors layer. Each builds a MotionCommand and sends it to the
 * motion task's FreeRTOS queue.
 *
 * Priority order (highest to lowest):
 *   STOP / PARK / DISABLE  → preempt everything
 *   TRACK                  → preempts SLEW
 *   SLEW / ENABLE          → normal
 *   SYNC                   → lowest
 * -------------------------------------------------------------------------- */

/* Request a slew to absolute axis targets (degrees) at the given velocities. */
void motors_motion_slew(float ra_target_deg, float dec_target_deg,
                                 float ra_velocity, float dec_velocity);

/* Request continuous tracking. Preempts any in-progress slew. */
void motors_motion_track(TrackingMode mode, float ra_velocity);

/*
 * Request continuous single-axis velocity motion.
 * rate_ra / rate_dec in deg/s — positive = forward, negative = reverse,
 * zero = stop that axis.  When both reach zero the status returns to READY.
 * Used by Alpaca MoveAxis, manual buttons, joystick, and guiding.
 */
void motors_motion_move_axis(float rate_ra, float rate_dec);

/* Request an immediate stop. Preempts everything. */
void motors_motion_stop(void);

/* Request park state. Preempts everything. */
void motors_motion_park(void);

/* Request disable state. Preempts everything. */
void motors_motion_disable(void);

/* Request enable (motors on, status READY). */
void motors_motion_enable(void);

/*
 * Align the internal position model to the given axis angles.
 * Does not move the motors — only updates the authoritative position.
 */
void motors_motion_sync(float ra_axis_deg, float dec_axis_deg);

/* Internal: create the command queue. Called from motors_motion_init(). */
void motors_motion_cmd_queue_create(void);

/* --------------------------------------------------------------------------
 * Hardware layer — STEP/DIR GPIO control.
 * -------------------------------------------------------------------------- */

typedef enum {
    MOTOR_DIRECTION_NEGATIVE = 0,
    MOTOR_DIRECTION_POSITIVE = 1,
} MotorDirection;

esp_err_t motors_motion_hw_init(void);

void motors_motion_hw_enable(void);

void motors_motion_hw_disable(void);

void motors_motion_hw_set_direction_ra(MotorDirection direction);

void motors_motion_hw_set_direction_dec(MotorDirection direction);

void motors_motion_hw_step_ra(void);

void motors_motion_hw_step_dec(void);
