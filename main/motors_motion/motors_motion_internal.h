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
    MOTION_CMD_STOP = 0,    /* highest — preempts everything */
    MOTION_CMD_PARK,        /* highest */
    MOTION_CMD_DISABLE,     /* highest */
    MOTION_CMD_SLEW,        /* high — preempts tracking (goto > track) */
    MOTION_CMD_TRACK,       /* high — preempts move-axis */
    MOTION_CMD_MOVE_AXIS,   /* normal */
    MOTION_CMD_ENABLE,      /* normal */
    MOTION_CMD_SYNC,        /* low — just align targets */
} MotionCommandType;

typedef struct {
    MotionCommandType type;
    float ra_target_deg;
    float dec_target_deg;
    float ra_velocity;
    float dec_velocity;
    TrackingMode tracking_mode;
    /*
     * Relative-move fields.
     * When `relative` is true, the motion task computes the absolute target
     * at processing time (position + delta) instead of using ra_target_deg
     * directly.  This guarantees correct enqueuing when the axis position
     * changes between the send and the dequeue of the command.
     */
    bool relative;
    float ra_delta_deg;
    float dec_delta_deg;
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

