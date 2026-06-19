/* LED — led_button_blink.c
 *
 * Purpose: double-blink the external LED for physical button feedback.
 *
 * Pattern: ON 500 ms → OFF 500 ms → ON 500 ms → OFF.
 * Non-blocking — chained esp_timer callbacks.
 */
#include "led.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_EXT_GPIO   GPIO_NUM_23
#define BLINK_MS       500

static esp_timer_handle_t s_chain_timer;

/*
 * Callback phases:
 *   1 — off, schedule phase 2 after 500 ms
 *   2 — on,  schedule phase 3 after 500 ms
 *   3 — off, done (no more timers)
 */
static void blink_step(void *arg) {
    int phase = (int)(uintptr_t) arg;

    /* Release the timer that fired us. */
    if (s_chain_timer) {
        esp_timer_delete(s_chain_timer);
        s_chain_timer = NULL;
    }

    if (phase == 1) {
        gpio_set_level(LED_EXT_GPIO, 0);
    } else if (phase == 2) {
        gpio_set_level(LED_EXT_GPIO, 1);
    } else {
        gpio_set_level(LED_EXT_GPIO, 0);
        return; /* phase 3 — sequence complete */
    }

    /* Schedule next phase. */
    esp_timer_create_args_t args = {
        .callback = blink_step,
        .arg = (void *)(uintptr_t)(phase + 1),
        .name = "blink_chain",
    };
    esp_timer_create(&args, &s_chain_timer);
    esp_timer_start_once(s_chain_timer, BLINK_MS * 1000UL);
}

bool led_is_busy(void) {
    return s_chain_timer != NULL;
}

void led_button_blink(void) {
    /* Cancel any in-progress blink sequence. */
    if (s_chain_timer) {
        esp_timer_stop(s_chain_timer);
        esp_timer_delete(s_chain_timer);
        s_chain_timer = NULL;
    }

    /* First blink: on now, off after 500 ms. */
    gpio_set_level(LED_EXT_GPIO, 1);

    esp_timer_create_args_t args = {
        .callback = blink_step,
        .arg = (void *) 1,
        .name = "blink_chain",
    };
    esp_timer_create(&args, &s_chain_timer);
    esp_timer_start_once(s_chain_timer, BLINK_MS * 1000UL);
}
