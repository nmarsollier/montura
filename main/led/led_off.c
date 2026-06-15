/* LED — led_off.c
 *
 * Purpose: turn the blue on-board LED off and cancel any pending timer.
 */
#include "led.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_GPIO GPIO_NUM_2

extern esp_timer_handle_t s_led_timer;

void led_off(void) {
    if (s_led_timer) {
        esp_timer_stop(s_led_timer);
        esp_timer_delete(s_led_timer);
        s_led_timer = NULL;
    }
    gpio_set_level(LED_GPIO, 0);
}
