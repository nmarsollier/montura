
/* Motors - motors_motion_task.c
 *
 * Purpose: FreeRTOS motion task — consumes MotionCommands from the queue
 * and advances axis positions one microstep at a time at the frequency
 * required to achieve the commanded angular velocity.
 *
 * The motion task is the SINGLE WRITER of motors_state position,
 * status, and tracking fields — all other code only reads them.
 *
 * Two execution paths, dispatched by command type:
 *   slewing_loop  — distance-bounded, ramped accel/decel, incremental scheduling
 *   tracking_loop — open-ended, constant velocity, absolute-time accumulator
 */

#include "motors_internal.h"

#include <math.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*
 * Task stack size — sized to accommodate deep call chains through step
 * helpers, condition checks, and FreeRTOS queue operations.
 */
#define MOTION_TASK_STACK_WORDS 4096
#define MOTION_TASK_PRIORITY    5

/* Step period ceiling — used when velocity is zero or unknown. */
#define MAX_STEP_PERIOD_US (10 * 1000 * 1000)

/*
 * Above this step period vTaskDelay yields the CPU; at or below it
 * a busy-wait with esp_timer keeps microsecond precision.
 */
#define BUSYWAIT_THRESHOLD_US 20000

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
 * Minimum slew velocity in centidegrees/second — floor for ramp curves.
 */
#define MIN_SLEW_CDS 80

/* Distance thresholds in centidegrees for ramp-profile selection. */
#define SHORT_SLEW_CDS   200   /* constant slow speed below this */
#define GENTLE_SLEW_CDS  800   /* cap target speed below this    */
#define FAST_SLEW_CDS   3500   /* aggressive profile above this  */

/*
 * Velocity profiles — 2 rows × 100 columns, each value is the
 * percentage of (target_vel − MIN_SLEW_CDS) added on top of the floor.
 *
 * Row 0 — gentle  (30 % linear accel, 40 % cruise, 30 % linear decel)
 * Row 1 — aggressive (10 % quadratic accel, 60 % cruise, 30 % linear decel)
 */
static const uint8_t VELOCITY_CURVE[2][100] = {
    {
        /* Row 0 — gentle profile */
        0, 3, 7, 10, 14, 17, 21, 24, 28, 31,
        34, 38, 41, 45, 48, 52, 55, 59, 62, 66,
        69, 72, 76, 79, 83, 86, 90, 93, 97, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 97, 93, 90, 86, 83, 79, 76, 72, 69,
        66, 62, 59, 55, 52, 48, 45, 41, 38, 34,
        31, 28, 24, 21, 17, 14, 10, 7, 3, 0,
    },
    {
        /* Row 1 — aggressive profile */
        0, 1, 5, 11, 20, 31, 44, 60, 79, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 97, 93, 90, 86, 83, 79, 76, 72, 69,
        66, 62, 59, 55, 52, 48, 45, 41, 38, 34,
        31, 28, 24, 21, 17, 14, 10, 7, 3, 0,
    },
};

/*
 * Compute the effective velocity for a single axis during a slew.
 *
 * Parameters are in centidegrees / centidegrees-per-second (×100 integers).
 * Only the return value is converted back to deg/s (float).
 */
static float ramp_velocity(int target_vel_cds, int position_cds,
                           int start_position_cds, int distance_cds) {
    if (target_vel_cds == 0)
        return 0.0f;

    if (distance_cds == 0)
        return (float) target_vel_cds / 100.0f;

    if (distance_cds < SHORT_SLEW_CDS)
        return (float) MIN_SLEW_CDS / 100.0f;

    int capped_vel = target_vel_cds;
    if (distance_cds < GENTLE_SLEW_CDS) {
        int speed_limit = MIN_SLEW_CDS * 4;
        if (capped_vel > speed_limit) capped_vel = speed_limit;
    }

    int curve = (distance_cds >= FAST_SLEW_CDS
                 && motors_state.status == MOTORS_STATUS_SLEWING)
                    ? 1
                    : 0;

    int travelled = position_cds - start_position_cds;
    if (travelled < 0) travelled = -travelled;
    int percent_index = (int) ((int64_t) travelled * 99 / distance_cds);

    int vel = MIN_SLEW_CDS + (capped_vel - MIN_SLEW_CDS) * VELOCITY_CURVE[curve][percent_index] / 100;
    return (float) vel / 100.0f;
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

    if (!s_motion.motion_active) {
        return false; /* sync stopped motion mid-iteration */
    }

    motors_hw_set_direction_ra(direction);
    motors_hw_step_ra();
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

    if (!s_motion.motion_active) {
        return false; /* sync stopped motion mid-iteration */
    }

    motors_hw_set_direction_dec(direction);
    motors_hw_step_dec();
    motors_state.dec_position = next_position;
    return true;
}

/* --------------------------------------------------------------------------
 * Only STOP / PARK / DISABLE may interrupt a running motion loop.
 * -------------------------------------------------------------------------- */
static bool is_terminal_command(MotionCommandType type) {
    return type == MOTION_CMD_STOP
           || type == MOTION_CMD_PARK
           || type == MOTION_CMD_DISABLE;
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

    /*
     * Slew / move-axis completion: both axes at target.
     *
     * Use active_cmd_type rather than motors_state.status because the
     * caller may have already set status to TRACKING (resume-after-slew
     * pattern in motors_slew_to_angle / motors_slew_axis_*) before the
     * motion task reaches the target.  Checking the authoritative
     * s_motion field guarantees completion is always detected.
     */
    if ((s_motion.active_cmd_type == MOTION_CMD_SLEW ||
         s_motion.active_cmd_type == MOTION_CMD_MOVE_AXIS) &&
        !ra_has_target && !dec_has_target) {
        motors_state.status = MOTORS_STATUS_READY;
        motors_state.tracking = TRACKING_NONE;
        s_motion.motion_active = false;

        return false;
    }

    /* External tracking stop. */
    if (motors_state.tracking == TRACKING_NONE &&
        motors_state.status == MOTORS_STATUS_TRACKING) {
        motors_state.status = MOTORS_STATUS_READY;
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
            motors_state.ra_velocity = cmd.ra_velocity;
            motors_state.dec_velocity = cmd.dec_velocity;
            motors_state.status = MOTORS_STATUS_SLEWING;
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
            motors_state.ra_velocity = cmd.ra_velocity;
            motors_state.dec_velocity = 0.0f;
            motors_state.status = MOTORS_STATUS_TRACKING;
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
            motors_state.ra_velocity = fabsf(cmd.ra_velocity);
            motors_state.dec_velocity = fabsf(cmd.dec_velocity);
            motors_state.status = MOTORS_STATUS_SLEWING;
            motors_state.tracking = TRACKING_NONE;

            s_motion.ra_target = (cmd.ra_velocity > 0.0f)
                                     ? motors_state.limits.ra_max
                                     : (cmd.ra_velocity < 0.0f)
                                         ? motors_state.limits.ra_min
                                         : motors_state.ra_position;
            s_motion.dec_target = (cmd.dec_velocity > 0.0f)
                                      ? motors_state.limits.dec_max
                                      : (cmd.dec_velocity < 0.0f)
                                          ? motors_state.limits.dec_min
                                          : motors_state.dec_position;

            s_motion.ra_start = motors_state.ra_position;
            s_motion.dec_start = motors_state.dec_position;
            s_motion.motion_active = true;

            break;

        case MOTION_CMD_STOP:
            s_motion.motion_active = false;
            motors_state.status = MOTORS_STATUS_READY;
            motors_state.tracking = TRACKING_NONE;
            break;

        case MOTION_CMD_PARK:
            s_motion.motion_active = false;
            motors_state.status = MOTORS_STATUS_PARKED;
            motors_state.tracking = TRACKING_NONE;
            break;

        case MOTION_CMD_DISABLE:
            s_motion.motion_active = false;
            motors_hw_disable();
            motors_state.status = MOTORS_STATUS_DISABLED;
            motors_state.tracking = TRACKING_NONE;
            break;

        case MOTION_CMD_ENABLE:
            s_motion.motion_active = false;
            motors_hw_enable();
            motors_state.status = MOTORS_STATUS_READY;
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
 * Synchronous position sync — called directly from the REST / mount layer.
 * Updates motors_state and internal targets without going through the queue
 * so mount_sync can return OK only after the position has been applied.
 * -------------------------------------------------------------------------- */
void motors_motion_sync_apply(float ra_axis_deg, float dec_axis_deg) {
    /* Stop any active motion — prevents the loop from fighting the update. */
    s_motion.motion_active = false;

    motors_state.ra_position = ra_axis_deg;
    motors_state.dec_position = dec_axis_deg;

    s_motion.ra_target = ra_axis_deg;
    s_motion.dec_target = dec_axis_deg;
    s_motion.ra_start = ra_axis_deg;
    s_motion.dec_start = dec_axis_deg;
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
     * Precompute total distances in centidegrees — constant for the
     * duration of a slew (only changes after preemption, at which point
     * we return and re-enter).
     */
    int distance_ra = (int) (fabsf(s_motion.ra_target - s_motion.ra_start) * 100.0f);
    int distance_dec = (int) (fabsf(s_motion.dec_target - s_motion.dec_start) * 100.0f);

    /*
     * Cache microstep resolution — sourced from TMC driver at loop entry.
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
                if (is_terminal_command(cmd.type)) {
                    xQueueReceive(motion_cmd_queue, &cmd, 0);
                    process_command(cmd);
                    if (!s_motion.motion_active) return;
                    next_ra_us = esp_timer_get_time();
                    next_dec_us = next_ra_us;
                    last_ramp_recalc_us = 0;
                    /* Refresh cached values after command switch. */
                    deg_per_step = motors_get_deg_per_microstep();
                    half_step = deg_per_step * 0.5f;
                    distance_ra = (int) (fabsf(s_motion.ra_target - s_motion.ra_start) * 100.0f);
                    distance_dec = (int) (fabsf(s_motion.dec_target - s_motion.dec_start) * 100.0f);
                    continue;
                }
            }

            if (!check_motion_conditions(deg_per_step)) break;
        }

        /* 3. Recalculate ramped velocities (throttled to every ~5 ms). */
        if (now - last_ramp_recalc_us > 5000) {
            int target_vel_ra = (int) (motors_state.ra_velocity * 100.0f);
            int target_vel_dec = (int) (motors_state.dec_velocity * 100.0f);
            int position_ra = (int) (motors_state.ra_position * 100.0f);
            int position_dec = (int) (motors_state.dec_position * 100.0f);

            float ra_vel = ramp_velocity(target_vel_ra, position_ra,
                                         (int) (s_motion.ra_start * 100.0f), distance_ra);
            float dec_vel = ramp_velocity(target_vel_dec, position_dec,
                                          (int) (s_motion.dec_start * 100.0f), distance_dec);
            ra_period = step_period_us(ra_vel);
            dec_period = step_period_us(dec_vel);
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
             * SLOW — step period > 20 ms.
             * Slews are distance-bounded so per-step jitter is harmless:
             * the ramp recalc compensates on the next iteration.
             */
            uint32_t sleep_ms = (wait_us / 1000 > 50) ? 50 : (uint32_t) (wait_us / 1000);
            if (sleep_ms < 1) sleep_ms = 1;
            vTaskDelay(pdMS_TO_TICKS(sleep_ms));
            continue;
        }

        if (wait_us > 100) {
            /*
             * FAST mode: busy-wait with periodic queue checks and yields.
             * Poll queue and yield every ~512 us to keep the scheduler alive.
             */
            while (esp_timer_get_time() < next_step) {
                if ((esp_timer_get_time() & 0x1FF) == 0) {
                    MotionCommand preempt_cmd;
                    if (xQueuePeek(motion_cmd_queue, &preempt_cmd, 0) == pdTRUE) {
                        if (is_terminal_command(preempt_cmd.type)) {
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
                if (is_terminal_command(cmd.type)) {
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
            if (motors_state.status != MOTORS_STATUS_TRACKING ||
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
                motors_state.status = MOTORS_STATUS_READY;
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
                    if (is_terminal_command(preempt_cmd.type)) {
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
                taskYIELD(); /* reset task WDT, let other tasks run */
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
    if (motors_state.status == MOTORS_STATUS_TRACKING && motors_state.tracking != TRACKING_NONE) {
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

void motors_motion_task_init(void) {
    xTaskCreate(
        motors_motion_task_run,
        "motors_motion",
        MOTION_TASK_STACK_WORDS,
        NULL,
        MOTION_TASK_PRIORITY,
        &s_motion_task_handle);

    /* Report stack high-water mark for diagnostics. */
    if (s_motion_task_handle != NULL) {
        UBaseType_t high_water = uxTaskGetStackHighWaterMark(s_motion_task_handle);
        ESP_LOGI("MOTORS_MOTION_TASK", "Stack high-water mark: %lu words (total %d)",
                 (unsigned long) high_water, MOTION_TASK_STACK_WORDS);
    }
}
