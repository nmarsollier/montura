/* LED — led_on.c
 *
 * Purpose: turn the blue on-board LED on, with optional auto-off timer.
 */
#include "led.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_GPIO GPIO_NUM_2

esp_timer_handle_t s_led_timer;

static void led_timer_callback(void *arg) {
    (void) arg;
    led_off();
}

void led_on(uint32_t duration_ms) {
    gpio_set_level(LED_GPIO, 1);

    if (duration_ms == 0)
        return;

    /* Cancel any pending timer before starting a new one. */
    if (s_led_timer) {
        esp_timer_stop(s_led_timer);
        esp_timer_delete(s_led_timer);
        s_led_timer = NULL;
    }

    esp_timer_create_args_t args = {
        .callback = led_timer_callback,
        .arg = NULL,
        .name = "led_timer",
    };
    esp_timer_create(&args, &s_led_timer);
    esp_timer_start_once(s_led_timer, duration_ms * 1000UL);
}
