/* Motors - motors_hw.c
 *
 * Purpose: TMC2209 STEP/DIR GPIO control.
 *
 * Hardware: NEMA 17 stepper motors driven by TMC2209 in STEP/DIR mode
 * with UART-configured microstepping and hardware interpolation.
 * Gear reduction: MOTOR_PULLEY_TEETH:AXIS_PULLEY_TEETH (see motors_internal.h).
 *
 * STEP pulse width is set above the TMC2209 minimum (~100 ns) with
 * margin for GPIO slew rate, keeping duty cycle low at max slew speed.
 */

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "motors.h"
#include "motors_internal.h"
#include "tmc/tmc.h"

#define MOTORS_ENABLE_GPIO GPIO_NUM_27
#define RA_STEP_GPIO GPIO_NUM_26
#define DEC_STEP_GPIO GPIO_NUM_25
#define RA_DIR_GPIO  GPIO_NUM_33
#define DEC_DIR_GPIO  GPIO_NUM_32

/*
 * STEP pulse width — set above TMC2209 minimum to absorb GPIO overhead.
 * HIGH_US + LOW_US = total pulse; must remain a small fraction of the
 * shortest step period at max slew speed.
 */
#define STEP_PULSE_HIGH_US 2
#define STEP_PULSE_LOW_US  2

/* TMC2209 EN pin is usually active-low. */
#define MOTORS_ENABLE_ACTIVE_LEVEL 0
#define MOTORS_ENABLE_INACTIVE_LEVEL 1

/* Cached last directions to avoid redundant GPIO writes. */
static int last_dir_ra = -1;
static int last_dir_dec = -1;

esp_err_t motors_hw_init(void) {
    const uint64_t pin_mask =
            (1ULL << RA_STEP_GPIO) |
            (1ULL << RA_DIR_GPIO) |
            (1ULL << DEC_STEP_GPIO) |
            (1ULL << DEC_DIR_GPIO) |
            (1ULL << MOTORS_ENABLE_GPIO);

    gpio_config_t config = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t result = gpio_config(&config);

    if (result != ESP_OK) {
        return result;
    }

    gpio_set_level(RA_STEP_GPIO, 0);
    gpio_set_level(DEC_STEP_GPIO, 0);
    esp_rom_delay_us(20);

    last_dir_ra = 0;
    last_dir_dec = 0;

    motors_hw_enable();
    tmc2209_hw_init();

    return ESP_OK;
}

void motors_hw_enable(void) {
    gpio_set_level(MOTORS_ENABLE_GPIO, MOTORS_ENABLE_ACTIVE_LEVEL);
}

void motors_hw_disable(void) {
    gpio_set_level(MOTORS_ENABLE_GPIO, MOTORS_ENABLE_INACTIVE_LEVEL);
}

void motors_hw_set_direction_ra(MotorDirection direction) {
    int dir = direction == MOTOR_DIRECTION_POSITIVE ? 0 : 1;
    if (last_dir_ra != dir) {
        last_dir_ra = dir;
        gpio_set_level(RA_DIR_GPIO, dir);
    }
}

void motors_hw_set_direction_dec(MotorDirection direction) {
    int dir = direction == MOTOR_DIRECTION_POSITIVE ? 1 : 0;
    if (last_dir_dec != dir) {
        last_dir_dec = dir;
        gpio_set_level(DEC_DIR_GPIO, dir);
    }
}

/*
 * Generate a STEP pulse on the RA axis.
 * Pulse width = STEP_PULSE_HIGH_US + STEP_PULSE_LOW_US.
 * Duty cycle stays low at max slew speed; well within TMC2209 spec.
 */
void motors_hw_step_ra() {
    gpio_set_level(RA_STEP_GPIO, 1);
    esp_rom_delay_us(STEP_PULSE_HIGH_US);
    gpio_set_level(RA_STEP_GPIO, 0);
    esp_rom_delay_us(STEP_PULSE_LOW_US);
}

void motors_hw_step_dec() {
    gpio_set_level(DEC_STEP_GPIO, 1);
    esp_rom_delay_us(STEP_PULSE_HIGH_US);
    gpio_set_level(DEC_STEP_GPIO, 0);
    esp_rom_delay_us(STEP_PULSE_LOW_US);
}
