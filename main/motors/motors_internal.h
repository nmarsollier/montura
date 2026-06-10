#pragma once

#include "motors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* =========================================================================
 * Motion command queue — thread-safe communication with the motion task.
 *
 * External callers (REST handlers, button poller) send MotionCommand
 * structs to the queue.  The motors task is the sole consumer and the
 * sole writer of motors_state position / status / tracking fields.
 * ========================================================================= */

typedef enum {
    MOTION_CMD_STOP = 0, /* preempts everything      */
    MOTION_CMD_PARK, /* preempts everything      */
    MOTION_CMD_DISABLE, /* preempts everything      */
    MOTION_CMD_SLEW, /* preempts TRACK/MOVE_AXIS */
    MOTION_CMD_TRACK, /* normal                   */
    MOTION_CMD_MOVE_AXIS, /* normal                   */
    MOTION_CMD_ENABLE, /* normal                   */
    MOTION_CMD_SYNC, /* lowest — align targets   */
} MotionCommandType;

typedef struct {
    MotionCommandType type;
    float ra_target_deg;
    float dec_target_deg;
    float ra_velocity;
    float dec_velocity;
    TrackingMode tracking_mode;
    bool relative;
    float ra_delta_deg;
    float dec_delta_deg;
} MotionCommand;

/* Queue handle — created by motors_init(), shared across the module. */
extern QueueHandle_t motion_cmd_queue;

/* Priority lookup (0 = highest). */
int motors_queue_priority(MotionCommandType type);

/* Send a MotionCommand to the back of the queue (FIFO). */
void motors_queue_send(MotionCommand *cmd);

/* Atomically discard every command in the queue. */
void motors_queue_clear(void);

/* Validate axis values against the configured inclusive limits. */
bool motors_is_valid_ra(float value);

bool motors_is_valid_dec(float value);

/* =========================================================================
 * Mechanical constants — hardware configuration.
 * ========================================================================= */
#define MOTOR_STEP_ANGLE_DEG     (1.8f)
#define MOTOR_FULL_STEPS_PER_REV ((int)(360.0f / MOTOR_STEP_ANGLE_DEG))
#define MOTOR_PULLEY_TEETH       (20)
#define AXIS_PULLEY_TEETH        (80)

/*
 * Motion calibration factor.
 *
 * Compensates for discrepancies between configured and actual step resolution.
 * When commanding 180° results in 90° physical movement, the factor is 2.0.
 *
 * Adjust this value until commanded angles match physical movement:
 *   - Mount moves too little → increase the factor
 *   - Mount moves too much   → decrease the factor
 *
 * Formula: factor = commanded_angle / actual_angle
 * Example:  2.0 = 180° / 90°
 */
#define MOTION_CALIBRATION_FACTOR 1.0f

/* =========================================================================
 * Microstep and step resolution — sourced from TMC hardware.
 *
 * The TMC2209 driver is the SINGLE source of truth for microstep count.
 * Use the inline helper below to always read the active value from the
 * TMC module's verified cache (no UART transaction needed on read).
 *
 * Fallback: if TMC not yet initialized, returns TMC_TARGET_MICROSTEPS (128).
 * ========================================================================= */

/*
 * Get the active microstep count from the TMC2209 driver.
 * Reads the cached value verified against hardware registers during init.
 *
 * @return Microstep count (e.g. 128), or 128 as fallback if TMC not ready.
 */
static inline uint16_t motors_get_microsteps(void) {
    extern uint16_t tmc2209_get_active_microsteps(void);
    uint16_t ms = tmc2209_get_active_microsteps();
    return (ms > 0) ? ms : 128;
}

/*
 * Angular displacement per microstep at the mount axis.
 * Computed at runtime using the TMC-verified microstep count.
 *
 * Formula: 360° / (200 full-steps × microsteps × (80/20) × calibration)
 *
 * With 128 µsteps and calibration 1.0: 360 / 102,400 = 0.003515625°
 */
static inline float motors_get_deg_per_microstep(void) {
    return 360.0f / ((float) MOTOR_FULL_STEPS_PER_REV *
                     (float) motors_get_microsteps() *
                     ((float) AXIS_PULLEY_TEETH / (float) MOTOR_PULLEY_TEETH) *
                     MOTION_CALIBRATION_FACTOR);
}

/* =========================================================================
 * Hardware layer — STEP/DIR GPIO control (motors_hw.c).
 * ========================================================================= */

typedef enum {
    MOTOR_DIRECTION_NEGATIVE = 0,
    MOTOR_DIRECTION_POSITIVE = 1,
} MotorDirection;

esp_err_t motors_hw_init(void);

void motors_hw_enable(void);

void motors_hw_disable(void);

void motors_hw_set_direction_ra(MotorDirection direction);

void motors_hw_set_direction_dec(MotorDirection direction);

void motors_hw_step_ra(void);

void motors_hw_step_dec(void);

/* =========================================================================
 * Task & queue lifecycle (motors_task.c, motors_queue.c).
 * ========================================================================= */

void motors_motion_task_start(void);

void motors_queue_create(void);
