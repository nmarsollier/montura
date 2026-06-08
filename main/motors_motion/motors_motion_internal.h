#pragma once

#include "motors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* --------------------------------------------------------------------------
 * Motion command queue — thread-safe communication with the motion task.
 *
 * External callers (motors layer, REST handlers, button poller) send
 * MotionCommand structs to the queue. The motors_motion task is the
 * sole consumer and the sole writer of motors_state fields that track
 * axis position.
 * -------------------------------------------------------------------------- */

typedef enum {
    MOTION_CMD_STOP = 0, /* highest priority — preempts everything */
    MOTION_CMD_PARK, /* highest priority */
    MOTION_CMD_DISABLE, /* highest priority */
    MOTION_CMD_TRACK, /* high priority — preempts slewing */
    MOTION_CMD_MOVE_AXIS, /* normal priority — continuous single-axis motion */
    MOTION_CMD_SLEW, /* normal priority */
    MOTION_CMD_ENABLE, /* normal priority */
    MOTION_CMD_SYNC, /* low priority — just align targets */
} MotionCommandType;

typedef struct {
    MotionCommandType type;
    float ra_target_deg;
    float dec_target_deg;
    float ra_velocity;
    float dec_velocity;
    TrackingMode tracking_mode;
} MotionCommand;

/* Queue handle created by motors_motion_init(), shared with motors_motion_commands.c. */
extern QueueHandle_t motion_cmd_queue;

/* Return the preemption priority for a command type (0 = highest). */
int motion_cmd_priority(MotionCommandType type);

/*
 * Internal helper — send a MotionCommand to the queue.
 * Uses xQueueSendToFront for high-priority commands (STOP, PARK, DISABLE, TRACK)
 * and xQueueSendToBack for normal-priority ones (SLEW, ENABLE).
 * Shared across per-use-case .c files within the motors_motion module.
 */
void motors_motion_cmd_send(MotionCommand *cmd, bool high_priority);
