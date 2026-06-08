/* MotorsMotion - motors_motion_task.c
 *
 * Purpose: FreeRTOS motion task — consumes commands from the priority queue
 * and advances axis positions one microstep at a time at the frequency
 * required to achieve the commanded angular velocity.
 *
 * The motion task is the SINGLE WRITER of motors_state position fields,
 * guaranteeing thread-safe position updates. Status transitions are also
 * performed here for commands that change operational state.
 *
 * Microstep resolution is sourced from the TMC2209 driver at runtime via
 * motors_get_deg_per_microstep(), ensuring it always matches hardware.
 *
 * Hardware: TMC2209 STEP/DIR GPIO control
 * Motor: NEMA 17 (200 full steps/rev, 1.8 deg/step)
 * Gear ratio: 20/80 (4:1) motor-to-axis
 *
 * Timing strategies (dual-path, dispatched by motion type):
 *
 *   SLEWING PATH (slewing_loop):
 *     - SLOW: step period > 20ms -> vTaskDelay yields CPU
 *     - FAST: step period <= 20ms -> busy-wait with esp_timer,
 *       checking the command queue every ~512 us for preemption
 *     - Ramped acceleration / deceleration for mechanical smoothness.
 *     - Uses incremental scheduling (next += period) — acceptable because
 *       slews are distance-bounded and short-lived.
 *
 *   TRACKING PATH (tracking_loop):
 *     - Absolute-time fractional accumulator: accumulates dt / period_us
 *       each iteration, emits floor(accumulator) steps, wraps the remainder.
 *     - Timing reference is esp_timer_get_time() — zero cumulative error
 *       over arbitrarily long tracking sessions.
 *     - No ramping (tracking velocities are constant and very low).
 */

#include "motors_motion.h"
#include "motors_motion_internal.h"
#include "motors_internal.h"

#include <math.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* Stack size for the motion task (words).  Bumped from 2048 to 4096
 * after observing a stray _invalid_pc_placeholder crash — a stack
 * overflow in the motion task would produce exactly that symptom
 * (the slewing_loop has deep call chains through step + condition
 * helpers and FreeRTOS queue operations). */
#define MOTION_TASK_STACK_WORDS 4096
#define MOTION_TASK_PRIORITY    5

/* Step period upper bound (us) used when velocity is zero or unknown. */
#define MAX_STEP_PERIOD_US (10 * 1000 * 1000) /* 10 seconds */

/*
 * Threshold for slewing vs tracking mode switching.
 * When step period > 20ms, use the efficient task-delay approach.
 * When step period <= 20ms, use busy-wait with esp_timer for precision.
 */
#define BUSYWAIT_THRESHOLD_US 20000 /* 20 ms */

static TaskHandle_t s_motion_task_handle = NULL;

/* --------------------------------------------------------------------------
 * Local motion state — active command being executed by the task.
 * -------------------------------------------------------------------------- */
static struct {
    MotionCommandType active_cmd_type;
    float ra_target;
    float dec_target;
    float ra_start; /* position captured at motion start (for ramps) */
    float dec_start;
    bool motion_active;
} s_motion;

/* --------------------------------------------------------------------------
 * Slew acceleration / deceleration
 * -------------------------------------------------------------------------- */

/*
 * Fixed acceleration / deceleration distance in degrees.
 *
 * Every slew reserves the first RAMP_DEGREES for acceleration and the
 * last RAMP_DEGREES for deceleration — regardless of total distance.
 * This guarantees visible, perceptible ramps even for short slews.
 *
 * When total distance < 2 × RAMP_DEGREES (i.e. < 16°) the ramp distance
 * is halved so the entire motion becomes a triangular profile: first
 * half accelerating, second half decelerating, no cruise phase.
 *
 *   Slew 30° :  8° ↑  +  14° →  +  8° ↓
 *   Slew 10° :  5° ↑  +   5° ↓        (triangular, 10 < 16)
 *   Slew  6° :  3° ↑  +   3° ↓        (triangular)
 */
#define RAMP_DEGREES 8.0f

/*
 * Minimum velocity factor during ramp.
 *
 * 0.2 = 20 % of target velocity.  Combined with the quadratic curve,
 * the ramp holds this floor for the first sqrt(0.2) ≈ 45 % of the
 * ramp distance — a long, flat initial phase that lets the rotor
 * overcome static friction before acceleration begins in earnest.
 * At 16 °/s the first step fires at 3.2 °/s (~1.1 ms period).
 */
#define MIN_RAMP_FACTOR 0.2f

/*
 * Compute the effective velocity for an axis during a slew, applying
 * acceleration and deceleration ramps.
 *
 * Ramp distance is fixed at RAMP_DEGREES (or total_dist / 2 when the
 * total is less than 2 × RAMP_DEGREES).  The curve is quadratic (t²) —
 * starts very flat and progressively steepens.  This produces a visibly
 * smooth ease-in / ease-out compared to a linear ramp.
 *
 * During TRACKING the ramp is bypassed — tracking speeds are too low to
 * benefit from ramping and the velocity must remain constant.
 */
static float ramped_velocity(float target_vel, float position,
                             float start_pos, float target_pos) {
    if (motors_state.status != MOUNT_STATUS_SLEWING)
        return target_vel;

    float total_dist = fabsf(target_pos - start_pos);
    if (total_dist < 1e-6f)
        return target_vel;

    float traveled = fabsf(position - start_pos);
    float remaining = fabsf(target_pos - position);

    /*
     * Fixed ramp distance: RAMP_DEGREES for slews ≥ 2× RAMP_DEGREES,
     * half the total for shorter slews (triangular velocity profile).
     */
    float ramp_dist;
    if (total_dist >= (RAMP_DEGREES * 2.0f)) {
        ramp_dist = RAMP_DEGREES;
    } else {
        ramp_dist = total_dist * 0.5f;
    }

    float ramp = 1.0f;

    if (traveled < ramp_dist) {
        /* Acceleration — quadratic ease-in (t²) */
        float t = traveled / ramp_dist;
        ramp = t * t;
        if (ramp < MIN_RAMP_FACTOR) ramp = MIN_RAMP_FACTOR;
    } else if (remaining < ramp_dist) {
        /* Deceleration — quadratic ease-out (t² mirrored) */
        float t = remaining / ramp_dist;
        ramp = t * t;
        if (ramp < MIN_RAMP_FACTOR) ramp = MIN_RAMP_FACTOR;
    }
    /* else: cruise phase — ramp stays at 1.0 */

    float velocity = target_vel * ramp;

    /*
     * Distance-based velocity ceiling for smooth short slews.
     * The ramp algorithm above uses the full target_vel internally so the
     * acceleration / deceleration shape is preserved; only the final output
     * is clamped so the axis never outruns the available ramp distance.
     */
    if (total_dist <= 1.0f && velocity > 3.0f) {
        return 3.0f;
    }
    if (total_dist <= 3.0f && velocity > 5.0f) {
        return 5.0f;
    }
    return velocity;
}

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
 * Pure step computation — separable from GPIO hardware for testability.
 *
 * Returns true if a step is needed. When true, *next_position is the
 * new axis position and *direction is the required motor direction.
 * -------------------------------------------------------------------------- */
static bool compute_next_step(float current, float target, float deg_per_step,
                              float *next_position, MotorDirection *direction) {
    float error = target - current;
    if (fabsf(error) < (deg_per_step * 0.5f))
        return false;

    if (error > 0.0f) {
        *next_position = current + deg_per_step;
        *direction = MOTOR_DIRECTION_POSITIVE;
    } else {
        *next_position = current - deg_per_step;
        *direction = MOTOR_DIRECTION_NEGATIVE;
    }
    return true;
}

/* --------------------------------------------------------------------------
 * Axis step helpers — advance position by one microstep toward target.
 *
 * Use compute_next_step() for the position calculation, then validate
 * limits, drive GPIO, and update motors_state.
 * Returns true when the axis is still moving, false when at target or limit.
 * -------------------------------------------------------------------------- */

static bool step_axis_ra(float target_deg, float deg_per_step) {
    float next_position;
    MotorDirection direction;

    if (!compute_next_step(motors_state.ra_position, target_deg,
                           deg_per_step, &next_position, &direction))
        return false;

    if (!motors_is_valid_ra(next_position)) {
        return false;
    }

    motors_motion_hw_set_direction_ra(direction);
    motors_motion_hw_step_ra();
    motors_state.ra_position = next_position;
    return true;
}

static bool step_axis_dec(float target_deg, float deg_per_step) {
    float next_position;
    MotorDirection direction;

    if (!compute_next_step(motors_state.dec_position, target_deg,
                           deg_per_step, &next_position, &direction))
        return false;

    if (!motors_is_valid_dec(next_position)) {
        return false;
    }

    motors_motion_hw_set_direction_dec(direction);
    motors_motion_hw_step_dec();
    motors_state.dec_position = next_position;
    return true;
}

/* --------------------------------------------------------------------------
 * Motion conditions check — returns false when the current motion should end.
 * -------------------------------------------------------------------------- */
static bool check_motion_conditions(float deg_per_step) {
    float half_step = deg_per_step * 0.5f;
    bool ra_has_target =
            fabsf(s_motion.ra_target - motors_state.ra_position) >= half_step;
    bool dec_has_target =
            fabsf(s_motion.dec_target - motors_state.dec_position) >= half_step;

    /* Slew completion: both axes at target. */
    if (motors_state.status == MOUNT_STATUS_SLEWING &&
        !ra_has_target && !dec_has_target) {
        motors_state.status = MOUNT_STATUS_READY;
        motors_state.tracking = TRACKING_NONE;
        s_motion.motion_active = false;

        return false;
    }

    /* External tracking stop. */
    if (motors_state.tracking == TRACKING_NONE &&
        motors_state.status == MOUNT_STATUS_TRACKING) {
        motors_state.status = MOUNT_STATUS_READY;
        s_motion.motion_active = false;

        return false;
    }

    return true;
}

/* --------------------------------------------------------------------------
 * Command processing — handle one MotionCommand and set up motion state.
 *
 * This is the ONLY place where motors_state transitions occur for
 * operational commands (SLEW, TRACK, STOP, PARK, etc.).
 * -------------------------------------------------------------------------- */
static void process_command(MotionCommand cmd) {
    s_motion.active_cmd_type = cmd.type;

    switch (cmd.type) {
        case MOTION_CMD_SLEW:
            motors_set_axis_velocity_ra(cmd.ra_velocity);
            motors_set_axis_velocity_dec(cmd.dec_velocity);
            motors_state.status = MOUNT_STATUS_SLEWING;
            motors_state.tracking = TRACKING_NONE;

            if (cmd.relative) {
                s_motion.ra_target = motors_state.ra_position + cmd.ra_delta_deg;
                s_motion.dec_target = motors_state.dec_position + cmd.dec_delta_deg;
            } else {
                s_motion.ra_target = cmd.ra_target_deg;
                s_motion.dec_target = cmd.dec_target_deg;
            }
            s_motion.ra_start = motors_state.ra_position;
            s_motion.dec_start = motors_state.dec_position;
            s_motion.motion_active = true;

            break;

        case MOTION_CMD_TRACK:
            motors_set_axis_velocity_ra(cmd.ra_velocity);
            motors_set_axis_velocity_dec(0.0f);
            motors_state.status = MOUNT_STATUS_TRACKING;
            motors_state.tracking = cmd.tracking_mode;

            /*
             * Tracking runs open-ended: target is set to the axis limit so
             * the loop never completes on its own — it only stops when an
             * external status change (STOP, PARK) is detected.
             *
             * Hemisphere selection: positive velocity → ra_max (northern),
             * negative velocity → ra_min (southern).  The sign is set by
             * motors_start_tracking based on site latitude.
             */
            s_motion.ra_target = (cmd.ra_velocity >= 0.0f)
                                     ? motors_state.limits.ra_max
                                     : motors_state.limits.ra_min;
            s_motion.dec_target = motors_state.dec_position;
            s_motion.ra_start = motors_state.ra_position;
            s_motion.dec_start = motors_state.dec_position;
            s_motion.motion_active = true;

            break;

        case MOTION_CMD_MOVE_AXIS:
            motors_set_axis_velocity_ra(fabsf(cmd.ra_velocity));
            motors_set_axis_velocity_dec(fabsf(cmd.dec_velocity));
            motors_state.status = MOUNT_STATUS_SLEWING;
            motors_state.tracking = TRACKING_NONE;

            /*
             * Continuous motion: each moving axis targets its limit in the
             * direction of travel so the loop never self-completes.
             * Motion stops only on the next command (rate = 0 or STOP).
             */
            s_motion.ra_target = (cmd.ra_velocity >= 0.0f)
                                     ? motors_state.limits.ra_max
                                     : motors_state.limits.ra_min;
            s_motion.dec_target = (cmd.dec_velocity >= 0.0f)
                                      ? motors_state.limits.dec_max
                                      : motors_state.limits.dec_min;
            s_motion.ra_start = motors_state.ra_position;
            s_motion.dec_start = motors_state.dec_position;
            s_motion.motion_active = true;

            break;

        case MOTION_CMD_STOP:
            s_motion.motion_active = false;
            motors_state.status = MOUNT_STATUS_READY;
            motors_state.tracking = TRACKING_NONE;
            break;

        case MOTION_CMD_PARK:
            s_motion.motion_active = false;
            motors_state.status = MOUNT_STATUS_PARKED;
            motors_state.tracking = TRACKING_NONE;
            break;

        case MOTION_CMD_DISABLE:
            s_motion.motion_active = false;
            motors_motion_hw_disable();
            motors_state.status = MOUNT_STATUS_DISABLED;
            motors_state.tracking = TRACKING_NONE;
            break;

        case MOTION_CMD_ENABLE:
            s_motion.motion_active = false;
            motors_motion_hw_enable();
            motors_state.status = MOUNT_STATUS_READY;
            motors_state.tracking = TRACKING_NONE;
            break;

        case MOTION_CMD_SYNC:
            s_motion.motion_active = false;
            motors_state.ra_position = cmd.ra_target_deg;
            motors_state.dec_position = cmd.dec_target_deg;

            /*
             * Also align the active motion targets so a future start
             * doesn't jump to stale coordinates.
             */
            s_motion.ra_target = cmd.ra_target_deg;
            s_motion.dec_target = cmd.dec_target_deg;
            s_motion.ra_start = cmd.ra_target_deg;
            s_motion.dec_start = cmd.dec_target_deg;

            break;
    }
}

/* --------------------------------------------------------------------------
 * Slewing & move-axis motion loop — incremental step scheduling.
 *
 * Selects the appropriate wait strategy each iteration:
 *   - vTaskDelay   when step period > 20 ms (slow slews)
 *   - busy-wait    when step period <= 20 ms (fast slewing)
 *
 * Polls the command queue at every opportunity so higher-priority
 * commands (TRACK during SLEW, STOP) can preempt immediately.
 *
 * Uses next += period scheduling with ramped acceleration / deceleration.
 * Acceptable for distance-bounded slews; NOT suitable for long-running
 * open-ended tracking — use tracking_loop() for that.
 * -------------------------------------------------------------------------- */
static void slewing_loop(void) {
    int64_t next_ra_us = esp_timer_get_time();
    int64_t next_dec_us = next_ra_us;
    int64_t last_ramp_recalc_us = 0;
    int64_t last_check_us = 0; /* throttle queue poll + conditions */
    int64_t last_wdt_pet_us = 0; /* throttle task watchdog reset */

    uint32_t ra_period = MAX_STEP_PERIOD_US;
    uint32_t dec_period = MAX_STEP_PERIOD_US;

    /*
     * Cache microstep resolution — sourced from TMC driver at loop entry.
     * Does not change during motion (hardware constant), so calling
     * motors_get_deg_per_microstep() once is safe and avoids 3 calls
     * per iteration (~27,000 calls/s during fast slewing).
     */
    float deg_per_step = motors_get_deg_per_microstep();
    float half_step = deg_per_step * 0.5f;

    while (s_motion.motion_active) {
        int64_t now = esp_timer_get_time();

        /*
         * Pet the task watchdog every ~1 s of continuous CPU time.
         * Fast-slewing busy-waits with taskYIELD() which never blocks
         * the task — long slews (e.g. 159° at 32 °/s ≈ 5 s) would
         * otherwise trigger the 5 s WDT timeout.  A brief vTaskDelay
         * actually blocks, resetting the watchdog.
         */
        if (now - last_wdt_pet_us > 1000000) {
            vTaskDelay(1);
            last_wdt_pet_us = esp_timer_get_time();
        }

        /*
         * 1+2. Throttled checks — queue poll and motion conditions.
         *
         * xQueueReceive is a FreeRTOS kernel call that enters a critical
         * section. Calling it on every microstep (~9,100/s at 16 °/s)
         * wastes CPU and adds per-step jitter.  500 us is frequent enough
         * for sub-ms command latency while cutting kernel calls by ~80 %.
         */
        if (now - last_check_us >= 500) {
            last_check_us = now;

            MotionCommand cmd;
            /*
             * Peek (don't consume) so commands that don't preempt stay
             * in the queue for the next motion.  Only xQueueReceive
             * when we're actually going to process the command.
             */
            if (xQueuePeek(motion_cmd_queue, &cmd, 0) == pdTRUE) {
                int incoming_prio = motion_cmd_priority(cmd.type);
                int current_prio = motion_cmd_priority(s_motion.active_cmd_type);

                if (incoming_prio < current_prio) {
                    xQueueReceive(motion_cmd_queue, &cmd, 0);
                    process_command(cmd);
                    if (!s_motion.motion_active) return;
                    next_ra_us = esp_timer_get_time();
                    next_dec_us = next_ra_us;
                    last_ramp_recalc_us = 0;
                    /* Refresh cached resolution after command switch. */
                    deg_per_step = motors_get_deg_per_microstep();
                    half_step = deg_per_step * 0.5f;
                    continue;
                }
            }

            if (!check_motion_conditions(deg_per_step)) break;
        }

        /* 3. Recalculate ramped velocities (throttled to every ~5 ms). */
        if (now - last_ramp_recalc_us > 5000) {
            float rv = ramped_velocity(motors_state.ra_velocity,
                                       motors_state.ra_position,
                                       s_motion.ra_start, s_motion.ra_target);
            float dv = ramped_velocity(motors_state.dec_velocity,
                                       motors_state.dec_position,
                                       s_motion.dec_start, s_motion.dec_target);
            ra_period = step_period_us(rv);
            dec_period = step_period_us(dv);
            last_ramp_recalc_us = now;
        }

        /* 4. Step axes that are due — uses cached deg_per_step. */
        bool ra_due = (now >= next_ra_us) &&
                      fabsf(s_motion.ra_target - motors_state.ra_position) >= half_step;
        bool dec_due = (now >= next_dec_us) &&
                       fabsf(s_motion.dec_target - motors_state.dec_position) >= half_step;

        if (ra_due) {
            step_axis_ra(s_motion.ra_target, deg_per_step);
            next_ra_us += ra_period;
            if (next_ra_us <= now) next_ra_us = now + ra_period;
        }

        if (dec_due) {
            step_axis_dec(s_motion.dec_target, deg_per_step);
            next_dec_us += dec_period;
            if (next_dec_us <= now) next_dec_us = now + dec_period;
        }

        /* 5. Smart wait — single strategy selection per iteration. */
        int64_t next_step = (next_ra_us < next_dec_us) ? next_ra_us : next_dec_us;
        int64_t wait_us = next_step - esp_timer_get_time();

        if (wait_us > BUSYWAIT_THRESHOLD_US) {
            /*
             * SLOW mode (tracking / slow slews).
             *
             * FreeRTOS vTaskDelay has ±10 ms jitter at the 100 Hz tick rate.
             * For sidereal tracking (~210 ms period) this would introduce
             * ~5 % period jitter — visible as periodic error in long exposures.
             *
             * Strategy: sleep via vTaskDelay until ~2 ms before the step,
             * then fine-wait the remainder with busy-wait for µs precision.
             * Command queue is checked during the fine-wait window.
             */
            int64_t fine_margin_us = 2000; /* 2 ms fine-wait window */

            if (wait_us > fine_margin_us) {
                int64_t sleep_us = wait_us - fine_margin_us;
                /* Cap at 50 ms to poll the command queue regularly. */
                uint32_t sleep_ms = (sleep_us / 1000 > 50) ? 50 : (uint32_t) (sleep_us / 1000);
                if (sleep_ms < 1) sleep_ms = 1;
                vTaskDelay(pdMS_TO_TICKS(sleep_ms));
            }

            /* Fine-wait: busy-wait the remaining margin for precise step timing.
             * Poll the command queue every ~512 us for preemption commands. */
            while (esp_timer_get_time() < next_step) {
                if ((esp_timer_get_time() & 0x1FF) == 0) {
                    MotionCommand preempt_cmd;
                    if (xQueuePeek(motion_cmd_queue, &preempt_cmd, 0) == pdTRUE) {
                        if (motion_cmd_priority(preempt_cmd.type) <
                            motion_cmd_priority(s_motion.active_cmd_type)) {
                            xQueueReceive(motion_cmd_queue, &preempt_cmd, 0);
                            process_command(preempt_cmd);
                            if (!s_motion.motion_active) return;
                            next_ra_us = esp_timer_get_time();
                            next_dec_us = next_ra_us;
                            last_ramp_recalc_us = 0;
                            break; /* exit fine-wait, re-enter outer loop */
                        }
                    }
                    taskYIELD();  /* reset task WDT, let other tasks run */
                }
            }
        } else if (wait_us > 100) {
            /*
             * FAST mode: busy-wait with periodic queue checks and yields.
             * Poll queue and yield every ~512 us to keep the scheduler alive.
             */
            while (esp_timer_get_time() < next_step) {
                if ((esp_timer_get_time() & 0x1FF) == 0) {
                    MotionCommand preempt_cmd;
                    if (xQueuePeek(motion_cmd_queue, &preempt_cmd, 0) == pdTRUE) {
                        if (motion_cmd_priority(preempt_cmd.type) <
                            motion_cmd_priority(s_motion.active_cmd_type)) {
                            xQueueReceive(motion_cmd_queue, &preempt_cmd, 0);
                            process_command(preempt_cmd);
                            if (!s_motion.motion_active) return;
                            next_ra_us = esp_timer_get_time();
                            next_dec_us = next_ra_us;
                            last_ramp_recalc_us = 0;
                            break; /* exit inner spin, re-enter outer loop */
                        }
                    }
                    taskYIELD();
                }
            }
        }
        /* else: wait_us <= 100 us — step is due immediately, just loop. */
    }
}

/* --------------------------------------------------------------------------
 * Tracking motion loop — absolute-time fractional accumulator + fine-wait.
 *
 * Designed for continuous open-ended tracking (sidereal, solar, lunar)
 * where timing precision must hold over arbitrarily long sessions.
 *
 * Scheduling strategy (hybrid sleep + fine-wait):
 *
 *   dt = now - last_time
 *   accumulator += dt / period_us
 *   while (accumulator >= 1.0):  step(),  accumulator -= 1.0
 *
 *   deadline = now + (1.0 - accumulator) * period_us    (µs-exact)
 *   if deadline - now > 2 ms:
 *       vTaskDelay most of it (capped 50 ms, yields CPU → near-zero consumption)
 *   fine-wait remaining margin with busy-wait + queue polling → µs precision
 *
 * Because every check references esp_timer_get_time() from a fixed
 * start point, there is zero cumulative timing error. The fine-wait
 * guarantees per-step jitter < 100 µs without consuming an extra
 * hardware timer or callback context.
 *
 * CPU: for sidereal tracking (~210 ms period), fine-wait burns ~2 ms
 *      of CPU every step → <1 % duty cycle. The remaining ~208 ms are
 *      spent in vTaskDelay with the CPU fully yielded to other tasks.
 *
 * Only RA is stepped during tracking; DEC velocity is always zero.
 * -------------------------------------------------------------------------- */
static void tracking_loop(void) {
    float deg_per_step = motors_get_deg_per_microstep();
    uint32_t period_us = step_period_us(motors_state.ra_velocity);

    /*
     * Fine-wait margin: sleep via vTaskDelay until this many µs before
     * the deadline, then busy-wait the remainder for µs precision.
     */
    const int64_t FINE_MARGIN_US = 2000; /* 2 ms */

    /* Fractional-step accumulator (double avoids single-precision drift). */
    double accumulator = 0.0;
    int64_t last_time_us = esp_timer_get_time();
    int64_t last_check_us = last_time_us;

    while (s_motion.motion_active) {
        int64_t now = esp_timer_get_time();
        int64_t dt_us = now - last_time_us;
        last_time_us = now;

        /* Accumulate fractional microsteps since last iteration. */
        if (dt_us > 0) {
            accumulator += (double) dt_us / (double) period_us;
        }

        /* ------------------------------------------------------------------
         * Throttled preemption & conditions check (every ~500 us).
         * ------------------------------------------------------------------ */
        if (now - last_check_us >= 500) {
            last_check_us = now;

            MotionCommand cmd;
            if (xQueuePeek(motion_cmd_queue, &cmd, 0) == pdTRUE) {
                int incoming_prio = motion_cmd_priority(cmd.type);
                int current_prio = motion_cmd_priority(s_motion.active_cmd_type);

                if (incoming_prio < current_prio) {
                    xQueueReceive(motion_cmd_queue, &cmd, 0);
                    process_command(cmd);
                    if (!s_motion.motion_active) return;

                    /* Reload tracking parameters for the new command. */
                    deg_per_step = motors_get_deg_per_microstep();
                    period_us = step_period_us(motors_state.ra_velocity);
                    accumulator = 0.0;
                    last_time_us = esp_timer_get_time();
                    last_check_us = last_time_us;
                    continue;
                }
            }

            if (!check_motion_conditions(deg_per_step)) break;

            /*
             * If a preempting command (e.g. SLEW) changed the status
             * from TRACKING to something else, exit so motion_loop()
             * redispatches to the correct loop (slewing_loop).
             */
            if (motors_state.status != MOUNT_STATUS_TRACKING ||
                motors_state.tracking == TRACKING_NONE) {
                break;
            }
        }

        /* ------------------------------------------------------------------
         * Emit accumulated whole steps.
         * ------------------------------------------------------------------ */
        while (accumulator >= 1.0) {
            if (!step_axis_ra(s_motion.ra_target, deg_per_step)) {
                /* Limit reached — terminate tracking. */
                s_motion.motion_active = false;
                motors_state.status = MOUNT_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                return;
            }
            accumulator -= 1.0;
        }

        /* ------------------------------------------------------------------
         * Hybrid sleep + fine-wait.
         *
         * Compute the exact deadline of the next whole step, sleep most of
         * the interval yielding the CPU, then fine-wait the final margin
         * for µs-precise step timing.
         *
         * If the deadline is still far away after one sleep chunk (e.g.
         * because the period is huge or the sleep was capped at 50 ms),
         * loop back to the outer while — re-accumulate dt, re-check
         * conditions, and re-sleep.  This prevents unbounded CPU spin
         * when velocity is near zero (degenerate tracking).
         * ------------------------------------------------------------------ */
        int64_t deadline = now + (int64_t) ((1.0 - accumulator) * (double) period_us);
        int64_t wait_us = deadline - esp_timer_get_time();

        if (wait_us > FINE_MARGIN_US) {
            /*
             * Sleep the bulk of the wait via vTaskDelay, capped at 50 ms
             * to keep the command queue responsive, then loop back.
             * The continue is critical: after a capped sleep the deadline
             * may still be far away — looping back re-checks conditions
             * and re-sleeps instead of falling into an unbounded fine-wait.
             */
            int64_t sleep_us = wait_us - FINE_MARGIN_US;
            uint32_t sleep_ms = (sleep_us / 1000 > 50)
                                    ? 50
                                    : (uint32_t) (sleep_us / 1000);
            if (sleep_ms < 1) sleep_ms = 1;
            vTaskDelay(pdMS_TO_TICKS(sleep_ms));
            continue;
        }

        /*
         * Fine-wait the remaining margin with busy-wait.
         * We only reach here when wait_us <= FINE_MARGIN_US (~2 ms),
         * so the spin is bounded and safe.
         *
         * Poll the command queue every ~512 µs for preemption commands.
         * For tracking step periods (≥ 100 ms), the 2 ms fine-wait window
         * represents ≤ 2 % CPU duty cycle.
         */
        while (esp_timer_get_time() < deadline) {
            if ((esp_timer_get_time() & 0x1FF) == 0) {
                MotionCommand preempt_cmd;
                if (xQueuePeek(motion_cmd_queue, &preempt_cmd, 0) == pdTRUE) {
                    if (motion_cmd_priority(preempt_cmd.type) <
                        motion_cmd_priority(s_motion.active_cmd_type)) {
                        xQueueReceive(motion_cmd_queue, &preempt_cmd, 0);
                        process_command(preempt_cmd);
                        if (!s_motion.motion_active) return;

                        /* Reload tracking parameters for the new command. */
                        deg_per_step = motors_get_deg_per_microstep();
                        period_us = step_period_us(motors_state.ra_velocity);
                        accumulator = 0.0;
                        last_time_us = esp_timer_get_time();
                        last_check_us = last_time_us;
                        break; /* exit fine-wait, re-enter outer loop */
                    }
                }
                taskYIELD();  /* reset task WDT, let other tasks run */
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * Motion loop dispatcher.
 *
 * Routes to the appropriate execution path based on mount status:
 *   - TRACKING (sidereal / solar / lunar) → tracking_loop()
 *     (absolute-time fractional accumulator, zero cumulative error)
 *   - Everything else (SLEWING, MOVE_AXIS, etc.) → slewing_loop()
 *     (incremental scheduling with ramps)
 * -------------------------------------------------------------------------- */
static void motion_loop(void) {
    if (motors_state.status == MOUNT_STATUS_TRACKING && motors_state.tracking != TRACKING_NONE) {
        tracking_loop();
    } else {
        slewing_loop();
    }
}

/* --------------------------------------------------------------------------
 * Motion task entry point.
 *
 * Blocks on the command queue when idle. When a motion-producing command
 * arrives (SLEW, TRACK, or MOVE_AXIS), enters motion_loop() which dispatches
 * to the appropriate execution path.
 * Non-motion commands (STOP, PARK, SYNC, etc.) are handled inline and
 * the task returns to blocking on the queue.
 * -------------------------------------------------------------------------- */
static void motors_motion_task_run(void *arg) {
    (void) arg;

    while (true) {
        MotionCommand cmd;
        if (xQueueReceive(motion_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE)
            continue;

        process_command(cmd);

        if (cmd.type == MOTION_CMD_SLEW || cmd.type == MOTION_CMD_TRACK ||
            cmd.type == MOTION_CMD_MOVE_AXIS) {
            motion_loop();
        }
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void motors_motion_init(void) {
    /* Create the command queue before the task so it's ready on first use. */
    motors_motion_cmd_queue_create();

    xTaskCreate(
        motors_motion_task_run,
        "motors_motion",
        MOTION_TASK_STACK_WORDS,
        NULL,
        MOTION_TASK_PRIORITY,
        &s_motion_task_handle);

    motors_motion_hw_init();

    /* Report stack high-water mark for diagnostics. */
    if (s_motion_task_handle != NULL) {
        UBaseType_t high_water = uxTaskGetStackHighWaterMark(s_motion_task_handle);
        ESP_LOGI("MOTORS_MOTION_TASK", "Stack high-water mark: %lu words (total %d)",
                 (unsigned long) high_water, MOTION_TASK_STACK_WORDS);
    }
}
