/* Buttons - buttons_poll.c
 *
 * Purpose: poll button inputs and trigger the corresponding actions.
 *
 * STOP:  rising edge → buttons_handle_stop().
 * HOME:  rising edge → record timestamp (no immediate action).
 *        If held ≥ 1 s → mount_sync(0,0) fires while still held.
 *        If released < 1 s → mount_home() fires on release.
 */
#include "buttons.h"

#include "led.h"
#include "mount.h"
#include "esp_timer.h"

typedef struct {
    bool last_stop;
    bool last_home;
    int64_t home_down_us;
    bool home_long_fired; /* prevent re-fire while held */
} InputState;

#define HOME_LONG_PRESS_US 1000000

static InputState input_state;

void buttons_init(void) {
    buttons_hw_init();
    input_state.last_stop = buttons_hw_is_stop_pressed();
    input_state.last_home = buttons_hw_is_home_pressed();
}

void buttons_poll_inputs(void) {
    bool stop_now = buttons_hw_is_stop_pressed();
    bool home_now = buttons_hw_is_home_pressed();

    /* STOP: rising edge → immediate. */
    if (stop_now && !input_state.last_stop) {
        buttons_handle_stop();
        led_on(500);
    }
    input_state.last_stop = stop_now;

    /* HOME */
    if (home_now && !input_state.last_home) {
        /* Rising edge — just record. */
        input_state.home_down_us = esp_timer_get_time();
        input_state.home_long_fired = false;
    } else if (home_now && input_state.last_home) {
        /* Held — check long-press threshold. */
        if (!input_state.home_long_fired &&
            (esp_timer_get_time() - input_state.home_down_us) >= HOME_LONG_PRESS_US) {
            mount_sync_position(0.0f, 0.0f);
            led_on(3000);
            input_state.home_long_fired = true;
        }
    } else if (!home_now && input_state.last_home) {
        /* Falling edge — short press (only if long-press didn't fire). */
        if (!input_state.home_long_fired) {
            mount_home();
            led_on(500);
        }
    }
    input_state.last_home = home_now;
}
