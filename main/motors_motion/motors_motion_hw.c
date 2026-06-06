/* MotorsMotion - motors_motion_hw.c
 *
 * Purpose: TMC2209 STEP/DIR GPIO control.
 *
 * Hardware configuration:
 *   - Driver: TMC2209 in STEP/DIR mode with UART configuration
 *   - Motor: NEMA 17 (200 full steps/rev, 1.8°/step)
 *   - Microsteps: 128 (configured via UART, with hardware interpolation enabled)
 *   - Gear ratio: 20/80 (4:1)
 *   - Processor: ESP32 (240 MHz)
 *
 * STEP pulse timing:
 *   - TMC2209 minimum STEP high time: ~100 ns (datasheet)
 *   - Using 2 µs high + 2 µs low for reliable operation at high speeds
 *   - Effective steps/rev = 200 × 128 × 4 × MOTION_CALIBRATION_FACTOR
 *     With calibration 1.0: 102,400 steps/rev
 *   - Max speed (32 °/s) = 32/360 × 102,400 ≈ 9,100 steps/s → period ~110 µs
 *   - Pulse (4 µs) = ~14.5 % duty cycle — within TMC2209 spec
 */

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "motors.h"
#include "motors_motion.h"
#include "tmc/tmc.h"

#define MOTORS_ENABLE_GPIO GPIO_NUM_27
#define RA_STEP_GPIO GPIO_NUM_26
#define DEC_STEP_GPIO GPIO_NUM_25
#define RA_DIR_GPIO  GPIO_NUM_33
#define DEC_DIR_GPIO  GPIO_NUM_32

/*
 * STEP pulse timing.
 * TMC2209 requires minimum ~100 ns high time.
 * We use 2 µs to safely accommodate GPIO overhead and provide
 * a clean pulse for both slow (tracking) and fast (slewing) rates.
 * Total pulse = HIGH_US + LOW_US.
 */
#define STEP_PULSE_HIGH_US 2
#define STEP_PULSE_LOW_US  2

/* TMC2209 EN pin is usually active-low. */
#define MOTORS_ENABLE_ACTIVE_LEVEL 0
#define MOTORS_ENABLE_INACTIVE_LEVEL 1

/* Cached last directions to avoid redundant GPIO writes. */
static int last_dir_ra = -1;
static int last_dir_dec = -1;

esp_err_t motors_motion_hw_init(void) {
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

    motors_motion_hw_enable();
    tmc2209_hw_init();

    return ESP_OK;
}

void motors_motion_hw_enable(void) {
    gpio_set_level(MOTORS_ENABLE_GPIO, MOTORS_ENABLE_ACTIVE_LEVEL);
}

void motors_motion_hw_disable(void) {
    gpio_set_level(MOTORS_ENABLE_GPIO, MOTORS_ENABLE_INACTIVE_LEVEL);
}

void motors_motion_hw_set_direction_ra(MotorDirection direction) {
    int dir = direction == MOTOR_DIRECTION_POSITIVE ? 1 : 0;
    if (last_dir_ra != dir) {
        last_dir_ra = dir;
        gpio_set_level(RA_DIR_GPIO, dir);
    }
}

void motors_motion_hw_set_direction_dec(MotorDirection direction) {
    int dir = direction == MOTOR_DIRECTION_POSITIVE ? 1 : 0;
    if (last_dir_dec != dir) {
        last_dir_dec = dir;
        gpio_set_level(DEC_DIR_GPIO, dir);
    }
}

/*
 * Generate a STEP pulse on the RA axis.
 * Pulse width is optimized for the TMC2209:
 *   - 2 µs high (well above 100 ns minimum)
 *   - 2 µs low  (ready for next step)
 *
 * At max slew (32 °/s = ~18,200 steps/s = 55 µs period),
 * the 4 µs pulse uses ~7% duty cycle — well within spec.
 */
void motors_motion_hw_step_ra() {
    gpio_set_level(RA_STEP_GPIO, 1);
    esp_rom_delay_us(STEP_PULSE_HIGH_US);
    gpio_set_level(RA_STEP_GPIO, 0);
    esp_rom_delay_us(STEP_PULSE_LOW_US);
}

void motors_motion_hw_step_dec() {
    gpio_set_level(DEC_STEP_GPIO, 1);
    esp_rom_delay_us(STEP_PULSE_HIGH_US);
    gpio_set_level(DEC_STEP_GPIO, 0);
    esp_rom_delay_us(STEP_PULSE_LOW_US);
}
