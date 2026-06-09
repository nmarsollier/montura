/* LED — led_init.c
 *
 * Purpose: configure the blue on-board LED GPIO and start in off state.
 */
#include "led.h"

#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_2

void led_init(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(LED_GPIO, 0);
}
