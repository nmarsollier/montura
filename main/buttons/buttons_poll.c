/* Buttons - buttons_poll.c
 *
 * Purpose: poll button inputs and trigger the corresponding actions.
 *
 * HOME button behaviour:
 *   Short press (< 1 s) → slew to home position.
 *   Long press  (≥ 1 s) → sync: set current position as the new home.
 */
#include "buttons.h"

#include "led.h"
#include "mount.h"
#include "esp_timer.h"

typedef struct {
    bool last_stop;
    bool last_home;
    int64_t home_press_start_us; /* 0 = not pressed */
} InputState;

#define HOME_LONG_PRESS_US 1000000   /* 1 second */

static InputState input_state;

void buttons_init(void) {
    buttons_hw_init();
    input_state.last_stop = buttons_hw_is_stop_pressed();
    input_state.last_home = buttons_hw_is_home_pressed();
    input_state.home_press_start_us = 0;
}

void buttons_poll_inputs(void) {
    bool stop_now = buttons_hw_is_stop_pressed();
    bool home_now = buttons_hw_is_home_pressed();

    /* STOP: rising edge → immediate action. */
    if (stop_now && !input_state.last_stop) {
        motors_button_stop();
        led_on(500);
    }
    input_state.last_stop = stop_now;

    /* HOME: rising edge → record timestamp.  Falling edge → dispatch. */
    if (home_now && !input_state.last_home) {
        input_state.home_press_start_us = esp_timer_get_time();
    } else if (!home_now && input_state.last_home) {
        int64_t held_us = esp_timer_get_time() - input_state.home_press_start_us;
        if (held_us >= HOME_LONG_PRESS_US) {
            mount_sync(0.0f, 0.0f);
            led_on(3000);
        } else {
            motors_home();
            led_on(500);
        }
        input_state.home_press_start_us = 0;
    }
    input_state.last_home = home_now;
}
