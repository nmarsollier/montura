/* MotorsMotion - motors_motion_task.c
 *
 * Purpose: FreeRTOS motion task — advances axis positions one microstep at a
 * time at the frequency required to achieve the commanded angular velocity.
 *
 * Microstep resolution is sourced from the TMC2209 driver at runtime via
 * motors_get_deg_per_microstep(), ensuring it always matches hardware.
 *
 * Hardware: TMC2209 STEP/DIR GPIO control
 * Motor: NEMA 17 (200 full steps/rev, 1.8°/step)
 * Gear ratio: 20/80 (4:1) motor-to-axis
 *
 * Timing strategies (dual-mode):
 *   - SLOW (tracking): step period > 20ms → vTaskDelay yields CPU
 *   - FAST (slewing):  step period <= 20ms → busy-wait with esp_timer for µs precision
 */
#include "motors_motion.h"
#include "motors_motion_internal.h"
#include "motors_internal.h"

#include <math.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MOTORS_MOTION_TASK";

/* Stack size for the motion task (words). */
#define MOTION_TASK_STACK_WORDS 2048
#define MOTION_TASK_PRIORITY    5

/* Step period upper bound (µs) used when velocity is zero or unknown. */
#define MAX_STEP_PERIOD_US (10 * 1000 * 1000) /* 10 seconds */

/*
 * Threshold for slewing vs tracking mode switching.
 * When step period > 20ms, use the efficient task-delay approach.
 * When step period <= 20ms, use busy-wait with esp_timer for precision.
 */
#define BUSYWAIT_THRESHOLD_US 20000 /* 20 ms */

static TaskHandle_t s_motion_task_handle = NULL;

/* Motion targets shared with motors_motion_sync.c via motors_motion_internal.h */
volatile float motors_motion_target_ra_deg = 0.0f;
volatile float motors_motion_target_dec_deg = 0.0f;

/* --------------------------------------------------------------------------
 * Step period helpers
 * -------------------------------------------------------------------------- */

/*
 * Convert an angular velocity (deg/s) to a step period in microseconds.
 * Uses the runtime microstep resolution from the TMC driver.
 * Returns MAX_STEP_PERIOD_US when velocity is effectively zero.
 */
static uint32_t step_period_us(float velocity_dps) {
    if (fabsf(velocity_dps) < 1e-9f)
        return MAX_STEP_PERIOD_US;

    float deg_per_step = motors_get_deg_per_microstep();
    float period_s = deg_per_step / fabsf(velocity_dps);
    uint32_t us = (uint32_t) (period_s * 1e6f);
    return (us == 0) ? 1 : us;
}

/* --------------------------------------------------------------------------
 * Axis step helpers — advance position by one microstep toward target.
 * Returns true when the axis is still moving, false when at target.
 * -------------------------------------------------------------------------- */

static bool step_axis_ra(float target_deg) {
    float error = target_deg - motors_state.ra_position;
    float deg_per_step = motors_get_deg_per_microstep();

    if (fabsf(error) < (deg_per_step * 0.5f))
        return false;

    float delta = (error > 0.0f) ? deg_per_step : -deg_per_step;
    float candidate = motors_state.ra_position + delta;

    if (!motors_is_valid_ra(candidate)) {
        ESP_LOGW(TAG, "RA limit reached at %.4f", candidate);
        return false;
    }

    motors_motion_hw_set_direction_ra(error > 0.0f ? MOTOR_DIRECTION_POSITIVE : MOTOR_DIRECTION_NEGATIVE);
    motors_motion_hw_step_ra();

    motors_state.ra_position = candidate;
    return true;
}

static bool step_axis_dec(float target_deg) {
    float error = target_deg - motors_state.dec_position;
    float deg_per_step = motors_get_deg_per_microstep();

    if (fabsf(error) < (deg_per_step * 0.5f))
        return false;

    float delta = (error > 0.0f) ? deg_per_step : -deg_per_step;
    float candidate = motors_state.dec_position + delta;

    if (!motors_is_valid_dec(candidate)) {
        ESP_LOGW(TAG, "DEC limit reached at %.4f", candidate);
        return false;
    }

    motors_motion_hw_set_direction_dec(error > 0.0f ? MOTOR_DIRECTION_POSITIVE : MOTOR_DIRECTION_NEGATIVE);
    motors_motion_hw_step_dec();

    motors_state.dec_position = candidate;
    return true;
}

/* --------------------------------------------------------------------------
 * Motion task
 *
 * Uses two timing strategies depending on step rate:
 *
 * 1. TRACKING/Slow (> 20ms period): Uses vTaskDelay() to yield the CPU.
 *    The TMC2209 hardware interpolator (intpol=1) internally generates
 *    smooth 256-microstep motion from each external STEP pulse.
 *
 * 2. SLEWING/Fast (<= 20ms period, down to ~55µs): Uses esp_timer
 *    busy-wait loop for precise microsecond-level step timing.
 * -------------------------------------------------------------------------- */
static void motors_motion_task_run(void *arg) {
    (void) arg;

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        float deg_per_step = motors_get_deg_per_microstep();

        ESP_LOGI(TAG, "Motion started: RA target=%.4f DEC target=%.4f (DPS=%.6f)",
                 motors_motion_target_ra_deg, motors_motion_target_dec_deg, deg_per_step);

        int64_t now = esp_timer_get_time();
        int64_t next_ra_step_us = now;
        int64_t next_dec_step_us = now;
        bool motion_active = true;

        while (motors_state.status == MOUNT_STATUS_SLEWING ||
               motors_state.status == MOUNT_STATUS_TRACKING) {
            float ra_target = motors_motion_target_ra_deg;
            float dec_target = motors_motion_target_dec_deg;

            uint32_t ra_period = step_period_us(motors_state.ra_velocity);
            uint32_t dec_period = step_period_us(motors_state.dec_velocity);

            now = esp_timer_get_time();

            float deg_per_step_local = motors_get_deg_per_microstep();
            bool ra_has_target =
                    fabsf(ra_target - motors_state.ra_position) >= (deg_per_step_local * 0.5f);

            bool dec_has_target =
                    fabsf(dec_target - motors_state.dec_position) >= (deg_per_step_local * 0.5f);

            /*
             * Check for completion (slew mode only).
             * Tracking mode runs open-ended until externally stopped.
             */
            if (motors_state.status == MOUNT_STATUS_SLEWING &&
                !ra_has_target && !dec_has_target) {
                motors_state.status = MOUNT_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                motors_state.last_update = esp_timer_get_time();
                motion_active = false;

                ESP_LOGI(TAG, "Slew complete: RA=%.4f DEC=%.4f",
                         motors_state.ra_position, motors_state.dec_position);
                break;
            }

            /*
             * Check for external tracking stop.
             */
            if ((motors_state.tracking == TRACKING_NONE ||
                 motors_state.tracking == TRACKING_MANUAL) &&
                motors_state.status == MOUNT_STATUS_TRACKING) {
                motors_state.status = MOUNT_STATUS_READY;
                motors_state.last_update = esp_timer_get_time();
                motion_active = false;

                ESP_LOGI(TAG, "Tracking stopped: RA=%.4f DEC=%.4f",
                         motors_state.ra_position, motors_state.dec_position);
                break;
            }

            /*
             * Choose timing strategy based on the shorter step period.
             * For tracking (~421ms period), yield CPU with vTaskDelay.
             * For slewing (down to ~55µs), use busy-wait precision loop.
             */
            uint32_t min_period = (ra_period < dec_period) ? ra_period : dec_period;

            if (min_period > BUSYWAIT_THRESHOLD_US) {
                /* === LOW-SPEED MODE (tracking, slow slews) ===
                 * Step period is long enough that vTaskDelay is adequate.
                 * The TMC2209 interpolator provides smooth motion internally.
                 */
                if (ra_has_target && now >= next_ra_step_us) {
                    step_axis_ra(ra_target);
                    next_ra_step_us += ra_period;
                    if (next_ra_step_us < now) {
                        next_ra_step_us = now + ra_period;
                    }
                }

                if (dec_has_target && now >= next_dec_step_us) {
                    step_axis_dec(dec_target);
                    next_dec_step_us += dec_period;
                    if (next_dec_step_us < now) {
                        next_dec_step_us = now + dec_period;
                    }
                }

                motors_state.last_update = now;
                vTaskDelay(pdMS_TO_TICKS(10));
            } else {
                /* === HIGH-SPEED MODE (slewing at max speed) ===
                 * Use a tight busy-wait loop with the shortest period to
                 * guarantee microsecond-level step timing precision.
                 */
                while (motion_active && (motors_state.status == MOUNT_STATUS_SLEWING ||
                                         motors_state.status == MOUNT_STATUS_TRACKING)) {
                    now = esp_timer_get_time();

                    /* Re-check targets on each iteration for responsiveness */
                    ra_target = motors_motion_target_ra_deg;
                    dec_target = motors_motion_target_dec_deg;
                    float dps_local = motors_get_deg_per_microstep();
                    ra_has_target = fabsf(ra_target - motors_state.ra_position) >= (dps_local * 0.5f);
                    dec_has_target = fabsf(dec_target - motors_state.dec_position) >= (dps_local * 0.5f);

                    /* Exit conditions */
                    if (!ra_has_target && !dec_has_target) {
                        if (motors_state.status == MOUNT_STATUS_SLEWING) {
                            motors_state.status = MOUNT_STATUS_READY;
                            motors_state.tracking = TRACKING_NONE;
                            ESP_LOGI(TAG, "Slew complete (fast): RA=%.4f DEC=%.4f",
                                     motors_state.ra_position, motors_state.dec_position);
                        }
                        break;
                    }

                    if ((motors_state.tracking == TRACKING_NONE ||
                         motors_state.tracking == TRACKING_MANUAL) &&
                        motors_state.status == MOUNT_STATUS_TRACKING) {
                        motors_state.status = MOUNT_STATUS_READY;
                        break;
                    }

                    /* Step RA if due */
                    if (ra_has_target && now >= next_ra_step_us) {
                        step_axis_ra(ra_target);
                        next_ra_step_us += ra_period;
                        if (next_ra_step_us <= now) {
                            next_ra_step_us = now + ra_period;
                        }
                    }

                    /* Step DEC if due */
                    if (dec_has_target && now >= next_dec_step_us) {
                        step_axis_dec(dec_target);
                        next_dec_step_us += dec_period;
                        if (next_dec_step_us <= now) {
                            next_dec_step_us = now + dec_period;
                        }
                    }

                    motors_state.last_update = now;

                    /* Small yield to prevent watchdog starvation on very long slews */
                    if ((now & 0x7FF) == 0) {
                        /* every ~2ms at 55µs periods */
                        taskYIELD();
                    }
                }

                /* Exit the outer loop since inner loop handles completion */
                if (!motion_active) break;
                if (motors_state.status != MOUNT_STATUS_SLEWING &&
                    motors_state.status != MOUNT_STATUS_TRACKING)
                    break;
            }
        }

        ESP_LOGI(TAG, "Motion task idle (status=%d)", motors_state.status);
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void motors_motion_init(void) {
    xTaskCreate(
        motors_motion_task_run,
        "motors_motion",
        MOTION_TASK_STACK_WORDS,
        NULL,
        MOTION_TASK_PRIORITY,
        &s_motion_task_handle);

    motors_motion_hw_init();

    ESP_LOGI(TAG, "Motion task created");
}

void motors_motion_start(float ra_target_deg, float dec_target_deg) {
    motors_motion_target_ra_deg = ra_target_deg;
    motors_motion_target_dec_deg = dec_target_deg;

    if (s_motion_task_handle != NULL)
        xTaskNotifyGive(s_motion_task_handle);
}
