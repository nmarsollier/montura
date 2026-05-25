/* Buttons - buttons_poll.c
 *
 * Purpose: poll button inputs and trigger the corresponding actions.
 */
#include "buttons.h"
#include "esp_log.h"

#include "mount.h"
#include "driver/gpio.h"

typedef struct {
    bool last_stop_pressed;
    bool last_home_pressed;
} InputState;

static InputState input_state = {
    .last_stop_pressed = false,
    .last_home_pressed = false,
};

void buttons_init(void) {
    buttons_hw_init();

    input_state.last_stop_pressed = buttons_hw_is_stop_pressed();

    input_state.last_home_pressed = buttons_hw_is_home_pressed();
}

void buttons_poll_inputs(void) {
    bool stop_pressed = buttons_hw_is_stop_pressed();

    bool home_pressed = buttons_hw_is_home_pressed();

    if (stop_pressed && !input_state.last_stop_pressed) {
        motors_button_stop();
    }
    input_state.last_stop_pressed = stop_pressed;

    if (home_pressed && !input_state.last_home_pressed) {
        motors_home();
    }
    input_state.last_home_pressed = home_pressed;
}
