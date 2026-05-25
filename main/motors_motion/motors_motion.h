#pragma once

#include "motors.h"

#include <stdint.h>

#include "esp_err.h"

/*
 * motors_motion — hardware motion execution layer.
 *
 * Owns the FreeRTOS motion task and all step-level execution.
 * Receives instructions from the motors module and drives axis positions
 * autonomously until the requested motion is complete or cancelled.
 *
 * Future home for TMC2209 STEP/DIR GPIO control.
 */

/*
 * Initialize the motion subsystem and create the background task.
 * Must be called once from motors_init() before any motion is requested.
 */
void motors_motion_init(void);

/*
 * Start motion toward absolute axis targets (degrees).
 * Sets the task targets and wakes the motion task.
 * The task reads motors_state.status to determine whether to slew or track.
 */
void motors_motion_start(float ra_target_deg, float dec_target_deg);

/*
 * Align internal motion targets to the given positions without starting motion.
 * Used after a sync operation to prevent spurious movement.
 */
void motors_motion_sync(float ra_axis_deg, float dec_axis_deg);

typedef enum {
    MOTOR_DIRECTION_NEGATIVE = 0,
    MOTOR_DIRECTION_POSITIVE = 1,
} MotorDirection;

esp_err_t motors_motion_hw_init(void);

void motors_motion_hw_enable(void);

void motors_motion_hw_disable(void);

void motors_motion_hw_set_direction_ra(MotorDirection direction);

void motors_motion_hw_set_direction_dec(MotorDirection direction);

void motors_motion_hw_step_ra();

void motors_motion_hw_step_dec();
