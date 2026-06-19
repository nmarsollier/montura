/* LED — led_slew_sync.c
 *
 * Purpose: sync the external LED to the mount slew state.
 *
 * ON while the mount is slewing (MOTORS_STATUS_SLEWING),
 * OFF in every other state.
 */
#include "led.h"

#include "driver/gpio.h"
#include "motors.h"

#define LED_EXT_GPIO GPIO_NUM_23

void led_slew_sync(void) {
    /* Don't override a blink sequence in progress. */
    if (led_is_busy()) return;

    MotorsState ms = motors_current_state();
    gpio_set_level(LED_EXT_GPIO, ms.status == MOTORS_STATUS_SLEWING ? 1 : 0);
}
