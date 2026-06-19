/* Buttons - buttons_poll.c
 *
 * Purpose: poll button inputs and trigger the corresponding actions.
 *
 * STOP:  rising edge → buttons_handle_stop().
 * HOME:  rising edge → mount_home().
 */
#include "buttons.h"

#include "led.h"
#include "mount.h"

typedef struct {
    bool last_stop;
    bool last_home;
} InputState;

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

    /* HOME: rising edge → immediate. */
    if (home_now && !input_state.last_home) {
        mount_home();
        led_on(500);
    }
    input_state.last_home = home_now;
}
