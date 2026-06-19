/* Buttons - buttons_hw.c
 *
 * Purpose: physical button GPIO initialisation and reading.
 */

#include "buttons.h"

#include "driver/gpio.h"

#define STOP_BUTTON_GPIO GPIO_NUM_19
#define HOME_BUTTON_GPIO GPIO_NUM_18

void buttons_hw_init(void) {
    gpio_reset_pin(STOP_BUTTON_GPIO);
    gpio_reset_pin(HOME_BUTTON_GPIO);
    gpio_set_direction(STOP_BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(HOME_BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_pullup_en(STOP_BUTTON_GPIO);
    gpio_pullup_en(HOME_BUTTON_GPIO);
    gpio_pulldown_dis(STOP_BUTTON_GPIO);
    gpio_pulldown_dis(HOME_BUTTON_GPIO);
}

bool buttons_hw_is_stop_pressed(void) {
    return gpio_get_level(STOP_BUTTON_GPIO) == 0;
}

bool buttons_hw_is_home_pressed(void) {
    return gpio_get_level(HOME_BUTTON_GPIO) == 0;
}
