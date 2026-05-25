// buttons_hw.c

#include "buttons.h"

#include "driver/gpio.h"

#define STOP_BUTTON_GPIO GPIO_NUM_13
#define HOME_BUTTON_GPIO GPIO_NUM_14

void buttons_hw_init(void) {
    gpio_reset_pin(GPIO_NUM_13);
    gpio_reset_pin(GPIO_NUM_14);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_NUM_14, GPIO_MODE_INPUT);
    gpio_pullup_en(GPIO_NUM_13);
    gpio_pullup_en(GPIO_NUM_14);
    gpio_pulldown_dis(GPIO_NUM_13);
    gpio_pulldown_dis(GPIO_NUM_14);
}


bool buttons_hw_is_stop_pressed(void) {
    return gpio_get_level(STOP_BUTTON_GPIO) == 0;
}

bool buttons_hw_is_home_pressed(void) {
    return gpio_get_level(HOME_BUTTON_GPIO) == 0;
}
