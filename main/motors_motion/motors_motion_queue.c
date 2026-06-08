/* MotorsMotion - motors_motion_queue.c
 *
 * Purpose: internal command-queue infrastructure for the motion task.
 *
 * Owns the FreeRTOS queue handle, priority lookup, queue lifecycle,
 * and the shared cmd_send helper used by every per-use-case .c file
 * in the motors_motion module.
 */

#include "motors_motion.h"
#include "motors_motion_internal.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "MOTORS_MOTION_CMD";

/* Single queue handle shared with the motion task via motors_motion_internal.h. */
QueueHandle_t motion_cmd_queue = NULL;

/* --------------------------------------------------------------------------
 * Command priority — lower number = higher preemption power.
 * -------------------------------------------------------------------------- */
int motion_cmd_priority(MotionCommandType type) {
    switch (type) {
        case MOTION_CMD_STOP:    return 0;
        case MOTION_CMD_PARK:    return 0;
        case MOTION_CMD_DISABLE: return 0;
        case MOTION_CMD_SLEW:      return 1;
        case MOTION_CMD_TRACK:     return 2;
        case MOTION_CMD_MOVE_AXIS: return 2;
        case MOTION_CMD_ENABLE:    return 2;
        case MOTION_CMD_SYNC:      return 3;
        default:                 return 99;
    }
}

/* --------------------------------------------------------------------------
 * Queue lifecycle — called from motors_motion_init().
 * -------------------------------------------------------------------------- */
void motors_motion_cmd_queue_create(void) {
    motion_cmd_queue = xQueueCreate(4, sizeof(MotionCommand));
    if (motion_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create motion command queue");
    }
}

/* --------------------------------------------------------------------------
 * Shared helper — send a command with the appropriate queue position.
 *
 * High-priority commands use xQueueSendToFront so they skip ahead of any
 * lower-priority commands already waiting.
 * -------------------------------------------------------------------------- */
void motors_motion_cmd_send(MotionCommand *cmd, bool high_priority) {
    if (motion_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Motion command queue not initialized");
        return;
    }

    if (high_priority) {
        /* Send to front — preempts queued lower-priority commands.
         * If the queue is full, drain the oldest (back) entry to make room. */
        if (xQueueSendToFront(motion_cmd_queue, cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
            MotionCommand discard;
            xQueueReceive(motion_cmd_queue, &discard, 0);
            xQueueSendToFront(motion_cmd_queue, cmd, pdMS_TO_TICKS(10));
        }
    } else {
        if (xQueueSendToBack(motion_cmd_queue, cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "Queue full, dropping cmd type=%d", cmd->type);
        }
    }
}
