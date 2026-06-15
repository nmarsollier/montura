/* LED — led_external_off.c
 *
 * Purpose: turn the external LED off and cancel any pending timer.
 */
#include "led.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_EXT_GPIO GPIO_NUM_23

extern esp_timer_handle_t s_led_ext_timer;

void led_external_off(void) {
    if (s_led_ext_timer) {
        esp_timer_stop(s_led_ext_timer);
        esp_timer_delete(s_led_ext_timer);
        s_led_ext_timer = NULL;
    }
    gpio_set_level(LED_EXT_GPIO, 0);
}
