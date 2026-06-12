/* LED — led_external_on.c
 *
 * Purpose: turn the external LED on, with optional auto-off timer.
 */
#include "led.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_EXT_GPIO GPIO_NUM_23

esp_timer_handle_t s_led_ext_timer;

static void led_ext_timer_callback(void *arg) {
    (void) arg;
    led_external_off();
}

void led_external_on(uint32_t duration_ms) {
    gpio_set_level(LED_EXT_GPIO, 1);

    if (duration_ms == 0)
        return;

    if (s_led_ext_timer) {
        esp_timer_stop(s_led_ext_timer);
        esp_timer_delete(s_led_ext_timer);
        s_led_ext_timer = NULL;
    }

    esp_timer_create_args_t args = {
        .callback = led_ext_timer_callback,
        .arg = NULL,
        .name = "led_ext_timer",
    };
    esp_timer_create(&args, &s_led_ext_timer);
    esp_timer_start_once(s_led_ext_timer, duration_ms * 1000UL);
}
