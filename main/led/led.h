#pragma once

#include <stdbool.h>
#include <stdint.h>

/* External LED — GPIO 23, active HIGH. */

void led_init(void);

/* Turn the external LED on.  duration_ms=0 stays on until led_external_off(). */
void led_external_on(uint32_t duration_ms);

void led_external_off(void);

/*
 * Quick double-blink (500 ms on, 500 ms off, 500 ms on) for
 * physical button feedback.  Non-blocking — uses esp_timer.
 */
void led_button_blink(void);

/* True while a blink sequence or timer-driven LED operation is in progress. */
bool led_is_busy(void);

/*
 * Sync the external LED to the current mount status.
 * ON while slewing (SLEWING status), OFF otherwise.
 * Does nothing while a blink / timer sequence is active.
 * Call periodically from the runtime loop.
 */
void led_slew_sync(void);
