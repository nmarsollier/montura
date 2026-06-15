#pragma once

#include <stdint.h>

/* Blue on-board LED — GPIO 2, active HIGH. */
/* External LED — GPIO 23, active HIGH. */

void led_init(void);

/*
 * Turn the on-board LED on.  If `duration_ms` is 0 the LED stays on until
 * led_off() is called.  If > 0 the LED automatically turns off
 * after that many milliseconds.
 */
void led_on(uint32_t duration_ms);

void led_off(void);

/*
 * Turn the external LED on.  If `duration_ms` is 0 the LED stays on until
 * led_external_off() is called.  If > 0 the LED automatically turns off
 * after that many milliseconds.
 */
void led_external_on(uint32_t duration_ms);

void led_external_off(void);
