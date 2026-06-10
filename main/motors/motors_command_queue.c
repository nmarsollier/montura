/* Motors - motors_queue.c
 *
 * Purpose: internal command-queue infrastructure for the motion task.
 *
 * Owns the FreeRTOS queue handle, priority lookup, queue lifecycle,
 * and the shared cmd_send helper used across the motors module.
 */

#include "motors.h"
#include "motors_internal.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "MOTORS_MOTION_CMD";

/* Single queue handle shared with the motion task via motors_motion_internal.h. */
QueueHandle_t motion_cmd_queue = NULL;

/* --------------------------------------------------------------------------
 * Command priority — lower number = higher preemption power.
 * -------------------------------------------------------------------------- */
int motors_queue_priority(MotionCommandType type) {
    switch (type) {
        case MOTION_CMD_STOP: return 0;
        case MOTION_CMD_PARK: return 0;
        case MOTION_CMD_DISABLE: return 0;
        case MOTION_CMD_SLEW: return 1;
        case MOTION_CMD_TRACK: return 2;
        case MOTION_CMD_MOVE_AXIS: return 2;
        case MOTION_CMD_ENABLE: return 2;
        case MOTION_CMD_SYNC: return 3;
        default: return 99;
    }
}

/* --------------------------------------------------------------------------
 * Queue lifecycle — called from motors_motion_init().
 * -------------------------------------------------------------------------- */
void motors_queue_create(void) {
    motion_cmd_queue = xQueueCreate(4, sizeof(MotionCommand));
    if (motion_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create motion command queue");
    }
}

/* --------------------------------------------------------------------------
 * Shared helper — send a command to the motion task queue.
 *
 * Every command is sent to the back (FIFO order).  STOP / PARK / DISABLE
 * callers reset the queue before sending so they always run immediately.
 * -------------------------------------------------------------------------- */
void motors_queue_send(MotionCommand *cmd) {
    if (motion_cmd_queue == NULL) {
        ESP_LOGE(TAG, "Motion command queue not initialized");
        return;
    }

    if (xQueueSendToBack(motion_cmd_queue, cmd, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full, dropping cmd type=%d", cmd->type);
    }
}

/* --------------------------------------------------------------------------
 * Public — atomically empty the command queue.
 * -------------------------------------------------------------------------- */
void motors_queue_clear(void) {
    if (motion_cmd_queue != NULL) {
        xQueueReset(motion_cmd_queue);
    }
}
