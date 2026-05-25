#include "tmc.h"
#include "tmc_internal.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * ESP32 UART port used to communicate with both TMC2209 drivers.
 * UART_NUM_2 is chosen because UART_NUM_0 is typically occupied by the
 * debug console and UART_NUM_1 may be in use by other peripherals.
 * The single-wire bus (TX and RX shorted) connects to the pins defined below.
 */
#define TMC_UART_NUM          UART_NUM_2

/*
 * TMC2209 UART GPIO pins.
 *   TX (GPIO_NUM_17) — write data to the drivers
 *   RX (GPIO_NUM_16) — read data from the drivers
 * Both pins are physically shorted on the single-wire bus as required
 * by the TMC2209 protocol.
 */
#define TMC_UART_TX_GPIO      GPIO_NUM_17
#define TMC_UART_RX_GPIO      GPIO_NUM_16

/*
 * TMC2209 UART baud rate.
 * 115200 bps is the manufacturer-recommended value for reliable
 * driver communication. The TMC2209 supports up to 500 kbps,
 * but 115200 offers the best balance between speed and stability
 * over longer cables.
 */
#define TMC_BAUD_RATE         115200

/*
 * UART addresses of the two axes on the TMC2209 bus.
 * Each TMC2209 driver has an 8-bit address configurable via the
 * MS1/MS2 pins (ADD0/ADD1 on TMC2209).
 *
 *   TMC_ADDR_RA  = 0x00 — Right Ascension axis
 *   TMC_ADDR_DEC = 0x03 — Declination axis
 *
 * Address 0x00 is obtained with ADD0=0, ADD1=0.
 * Address 0x03 is obtained with ADD0=1, ADD1=1.
 */
#define TMC_ADDR_RA           0x00
#define TMC_ADDR_DEC          0x03

/*
 * TMC2209 internal registers accessible via UART.
 * Reference: TMC2209 datasheet (Trinamic), section "Register Map".
 *
 *   GCONF      (0x00) — General driver configuration
 *   IFCNT      (0x02) — Received UART frame counter (read-only).
 *                        Increments by 1 on each successful write.
 *                        Useful for verifying the driver is responding.
 *   IOIN       (0x06) — Physical pin states (ENN, MS1, MS2, etc.)
 *   IHOLD_IRUN (0x10) — Hold current (IHOLD), run current (IRUN),
 *                        and transition delay (IHOLDDELAY).
 *   CHOPCONF   (0x6C) — Chopper configuration: MRES (microstep resolution),
 *                        operating mode (StealthChop / SpreadCycle),
 *                        interpolation, etc.
 */
#define TMC_REG_GCONF         0x00
#define TMC_REG_IFCNT         0x02
#define TMC_REG_IOIN          0x06
#define TMC_REG_IHOLD_IRUN    0x10
#define TMC_REG_CHOPCONF      0x6C

/*
 * Target microstep count to configure on both axes.
 * 256 microsteps per full step are used to obtain the maximum
 * resolution offered by the TMC2209 via UART.
 *
 * Hardware interpolation (intpol=1 in CHOPCONF) is also enabled,
 * meaning the driver internally generates 256 smooth microsteps
 * from each external STEP pulse, regardless of the actual MRES
 * value. This provides:
 *   - Ultra-smooth motion even at low tracking speeds
 *   - Reduced vibration and audible noise
 *   - Better response at low STEP frequencies
 */
#define TMC_TARGET_MICROSTEPS 256

/*
 * Motor current configuration (IHOLD_IRUN register, addr 0x10).
 *
 * The TMC2209 uses the IHOLD_IRUN register to control motor current
 * through three fields:
 *
 *   IHOLD      (bits 0-4)   — Hold current (motor stopped)
 *   IRUN       (bits 16-20) — Run current (motor moving)
 *   IHOLDDELAY (bits 8-11)  — IRUN-to-IHOLD transition delay
 *
 * RMS current calculation formula:
 *
 *   I_RMS = (register_value / 32) * I_MAX_DRIVER
 *
 * where I_MAX_DRIVER is the driver's maximum current determined by
 * the sense resistor (Rsense). For a typical NEMA 17 with Rsense=0.11 Ohm:
 *
 *   I_MAX_DRIVER = 0.22 V / 0.11 Ohm = 2.0 A (peak)
 *   I_MAX_RMS    = 2.0 A / sqrt(2) = 1.41 A RMS
 *
 * With IRUN = 14:  I_RMS = (14/32) * 1.41 A = 617 mA RMS
 * With IHOLD = 6:  I_RMS = (6/32) * 1.41 A = 264 mA RMS
 *
 * These values are deliberately below the motor's rated maximum
 * (typical NEMA 17 ~1.4 A) to:
 *   - Prevent motor and driver overheating
 *   - Reduce power consumption during long tracking sessions
 *   - Maintain sufficient torque to move the mount without saturation
 *
 * Reference: TMC2209 datasheet, Table 8.40 (IHOLD_IRUN).
 */
#define TMC_IHOLD       6   /* Hold current:  ~264 mA RMS */
#define TMC_IRUN       14   /* Run current:   ~617 mA RMS */
#define TMC_IHOLDDELAY  1   /* Standard IRUN->IHOLD delay */

/*
 * Bit masks for the TMC2209 GCONF register.
 * Reference: TMC2209 datasheet, Table 8.1 (GCONF).
 *
 *   GCONF_I_SCALE_ANALOG (Bit 0)
 *       1 — Motor current is controlled by the analog voltage on the
 *           VREF pin (external potentiometer).
 *       0 — Motor current is controlled digitally via the IHOLD_IRUN
 *           register over UART. Do NOT use the VREF pin.
 *       Forced to 0 in this configuration so current control is
 *       exclusively through software.
 *
 *   GCONF_MSTEP_REG_SELECT (Bit 7)
 *       1 — Microstep resolution (MRES) is set via the CHOPCONF register
 *           over UART (software control).
 *       0 — Resolution is determined by the MS1/MS2 pins.
 *       Forced to 1 to allow changing microsteps dynamically without
 *       hardware modifications.
 */
#define GCONF_I_SCALE_ANALOG   (1U << 0)
#define GCONF_MSTEP_REG_SELECT (1U << 7)


static const char *TAG = "TMC_INIT";

/*
 * Logical representation of a mount axis.
 * Each instance associates a human-readable name with the UART address
 * of the TMC2209 driver that controls that axis.
 *
 *   name    — Axis identifier: "RA" (Right Ascension) or "DEC" (Declination)
 *   address — 8-bit UART address of the TMC2209 on the single-wire bus
 *             (see TMC_ADDR_RA and TMC_ADDR_DEC)
 */
typedef struct {
    const char *name;
    uint8_t address;
} TmcAxis;

static const TmcAxis tmc_axes[] = {
    {.name = "RA", .address = TMC_ADDR_RA},
    {.name = "DEC", .address = TMC_ADDR_DEC},
};

static uint8_t tmc_crc(const uint8_t *data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (((crc >> 7) ^ (byte & 0x01)) != 0) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
            byte >>= 1;
        }
    }
    return crc;
}

static bool tmc_mres_to_microsteps(uint8_t mres, uint16_t *microsteps) {
    switch (mres) {
        case 0: *microsteps = 256;
            return true;
        case 1: *microsteps = 128;
            return true;
        case 2: *microsteps = 64;
            return true;
        case 3: *microsteps = 32;
            return true;
        case 4: *microsteps = 16;
            return true;
        case 5: *microsteps = 8;
            return true;
        case 6: *microsteps = 4;
            return true;
        case 7: *microsteps = 2;
            return true;
        case 8: *microsteps = 1;
            return true;
        default: return false;
    }
}

static bool tmc_microsteps_to_mres(uint16_t microsteps, uint8_t *mres) {
    switch (microsteps) {
        case 256: *mres = 0;
            return true;
        case 128: *mres = 1;
            return true;
        case 64: *mres = 2;
            return true;
        case 32: *mres = 3;
            return true;
        case 16: *mres = 4;
            return true;
        case 8: *mres = 5;
            return true;
        case 4: *mres = 6;
            return true;
        case 2: *mres = 7;
            return true;
        case 1: *mres = 8;
            return true;
        default: return false;
    }
}

static esp_err_t tmc_read_register(uint8_t address, uint8_t reg, uint32_t *value) {
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t request[4] = {
        0x05,
        address,
        (uint8_t) (reg & 0x7F),
        0x00,
    };

    request[3] = tmc_crc(request, 3);

    uart_flush_input(TMC_UART_NUM);

    int written = uart_write_bytes(TMC_UART_NUM, request, sizeof(request));
    if (written != (int) sizeof(request)) {
        return ESP_FAIL;
    }

    uart_wait_tx_done(TMC_UART_NUM, pdMS_TO_TICKS(10));

    // ABSORB ECHO: Read back the 4 bytes the ESP32 sent that bounced on the single-wire bus
    uint8_t echo_buffer[4];
    uart_read_bytes(TMC_UART_NUM, echo_buffer, sizeof(request), pdMS_TO_TICKS(10));

    // Read the actual response sent by the TMC2209 driver
    uint8_t response[32];
    int len = 0;
    TickType_t start = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(100);

    while ((xTaskGetTickCount() - start) < timeout && len < (int) sizeof(response)) {
        int read = uart_read_bytes(
            TMC_UART_NUM,
            response + len,
            sizeof(response) - len,
            pdMS_TO_TICKS(5));

        if (read > 0) {
            len += read;
        }
    }

    if (len < 8) {
        return ESP_ERR_TIMEOUT;
    }

    for (int i = 0; i <= len - 8; i++) {
        uint8_t *frame = &response[i];

        if (frame[0] != 0x05) continue;
        if ((frame[2] & 0x7F) != (reg & 0x7F)) continue;
        if (tmc_crc(frame, 7) != frame[7]) continue;

        *value = ((uint32_t) frame[3] << 24) |
                 ((uint32_t) frame[4] << 16) |
                 ((uint32_t) frame[5] << 8) |
                 ((uint32_t) frame[6]);

        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t tmc_write_register(uint8_t address, uint8_t reg, uint32_t value) {
    uint8_t request[8] = {
        0x05,
        address,
        (uint8_t) (reg | 0x80),
        (uint8_t) (value >> 24),
        (uint8_t) (value >> 16),
        (uint8_t) (value >> 8),
        (uint8_t) value,
        0x00,
    };

    request[7] = tmc_crc(request, 7);

    uart_flush_input(TMC_UART_NUM);

    int written = uart_write_bytes(TMC_UART_NUM, request, sizeof(request));
    if (written != (int) sizeof(request)) {
        return ESP_FAIL;
    }

    uart_wait_tx_done(TMC_UART_NUM, pdMS_TO_TICKS(10));

    // ABSORB ECHO: Read back the 8 written bytes to fully flush the receiver FIFO
    uint8_t echo_buffer[8];
    uart_read_bytes(TMC_UART_NUM, echo_buffer, sizeof(request), pdMS_TO_TICKS(10));

    return ESP_OK;
}

static esp_err_t tmc_read_microsteps(const TmcAxis *axis, uint32_t *chopconf, uint16_t *microsteps) {
    if (axis == NULL || chopconf == NULL || microsteps == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t result = tmc_read_register(axis->address, TMC_REG_CHOPCONF, chopconf);
    if (result != ESP_OK) {
        return result;
    }

    uint8_t mres = (uint8_t) ((*chopconf >> 24) & 0x0F);
    if (!tmc_mres_to_microsteps(mres, microsteps)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

static esp_err_t tmc_set_microsteps(const TmcAxis *axis, uint16_t microsteps) {
    if (axis == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t mres = 0;
    if (!tmc_microsteps_to_mres(microsteps, &mres)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t chopconf = 0;
    uint16_t current_microsteps = 0;

    esp_err_t result = tmc_read_microsteps(axis, &chopconf, &current_microsteps);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "%s failed reading CHOPCONF before update: %s", axis->name, esp_err_to_name(result));
        return result;
    }

    // 2. Clear the old MRES mask (bits 24-27) and inject the new value
    uint32_t updated = (chopconf & ~(0x0FU << 24)) | ((uint32_t) mres << 24);

    // =========================================================================
    // ASTRONOMICAL CONFIGURATION: FORCE STEALTHCHOP AND INTERPOLATION
    // =========================================================================

    // Clear Bit 14 (en_spreadcycle = 0) -> Force ultra-quiet StealthChop mode
    updated &= ~(1U << 14);

    // Set Bit 28 (intpol = 1) -> Enable hardware interpolation to 256 microsteps
    updated |= (1U << 28);

    // =========================================================================

    ESP_LOGI(TAG, "%s: Applying CHOPCONF (MRES=%u, StealthChop=ON, Intpol=ON)", axis->name, mres);

    // 3. Write the optimised CHOPCONF register to the driver
    result = tmc_write_register(axis->address, TMC_REG_CHOPCONF, updated);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "%s failed writing CHOPCONF: %s", axis->name, esp_err_to_name(result));
        return result;
    }

    // 4. Hardware latch verification and validation
    uint32_t verify_chopconf = 0;
    uint16_t verify_microsteps = 0;

    result = tmc_read_microsteps(axis, &verify_chopconf, &verify_microsteps);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "%s failed verifying microsteps: %s", axis->name, esp_err_to_name(result));
        return result;
    }

    if (verify_microsteps != microsteps) {
        ESP_LOGW(TAG, "%s microsteps not latched, expected=%u actual=%u", axis->name, microsteps, verify_microsteps);
        return ESP_ERR_INVALID_RESPONSE;
    }

    /*
     * Update the global cache with the verified hardware value.
     * Both axes are configured identically, so storing the last
     * verified value is safe.
     */
    tmc2209_set_active_microsteps(verify_microsteps);

    ESP_LOGI(TAG, "%s microsteps applied: %u (Hardware-interpolated to 256, cache=%u)",
             axis->name, verify_microsteps, tmc2209_get_active_microsteps());
    return ESP_OK;
}

static esp_err_t tmc_init_driver(const TmcAxis *axis) {
    uint32_t gconf = 0;
    uint32_t ifcnt_before = 0;
    uint32_t ifcnt_after = 0;
    esp_err_t result;

    ESP_LOGI(TAG, "--- Initialising axis %s [Addr: 0x%02X] ---", axis->name, axis->address);

    // 1. Monitor internal frame counter before making changes
    tmc_read_register(axis->address, TMC_REG_IFCNT, &ifcnt_before);

    // 2. Read initial GCONF state
    result = tmc_read_register(axis->address, TMC_REG_GCONF, &gconf);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: UART read failure on GCONF", axis->name);
        return result;
    }

    // 3. GCONF modifications:
    //    - Enable mstep_reg_select (Bit 7) -> Allow microstep changes via software
    //    - Disable i_scale_analog (Bit 0) -> Turn off physical VREF, ignore potentiometer
    gconf |= GCONF_MSTEP_REG_SELECT;
    gconf &= ~GCONF_I_SCALE_ANALOG;

    result = tmc_write_register(axis->address, TMC_REG_GCONF, gconf);
    if (result != ESP_OK) return result;

    // 4. Configure motor current using the constants defined above
    uint32_t ihold_irun_val = (uint32_t) TMC_IHOLD |
                              ((uint32_t) TMC_IHOLDDELAY << 8) |
                              ((uint32_t) TMC_IRUN << 16);

    result = tmc_write_register(axis->address, TMC_REG_IHOLD_IRUN, ihold_irun_val);
    if (result != ESP_OK) return result;

    // 5. Verify hardware IFCNT increment (GCONF + IHOLD_IRUN = +2 successful writes)
    tmc_read_register(axis->address, TMC_REG_IFCNT, &ifcnt_after);
    ESP_LOGI(TAG, "%s: IFCNT increment verified: %lu -> %lu",
             axis->name, ifcnt_before & 0xFF, ifcnt_after & 0xFF);

    // 6. Set target microsteps
    result = tmc_set_microsteps(axis, TMC_TARGET_MICROSTEPS);

    return result;
}

static void tmc_log_current_status(const TmcAxis *axis) {
    uint32_t gconf = 0;
    uint32_t chopconf = 0;
    uint32_t ioin = 0;
    uint16_t msteps = 0;

    // Read current registers directly from the chip
    esp_err_t r1 = tmc_read_register(axis->address, TMC_REG_GCONF, &gconf);
    esp_err_t r2 = tmc_read_register(axis->address, TMC_REG_CHOPCONF, &chopconf);
    esp_err_t r3 = tmc_read_register(axis->address, TMC_REG_IOIN, &ioin);

    if (r1 != ESP_OK || r2 != ESP_OK || r3 != ESP_OK) {
        ESP_LOGE(TAG, "[%s] Unable to read current UART status", axis->name);
        return;
    }

    // Extract microsteps from the read CHOPCONF bits
    uint8_t mres = (uint8_t) ((chopconf >> 24) & 0x0F);
    tmc_mres_to_microsteps(mres, &msteps);

    // Decode critical control flags
    bool mstep_by_uart = (gconf & GCONF_MSTEP_REG_SELECT) != 0;
    bool current_by_vref = (gconf & GCONF_I_SCALE_ANALOG) != 0;

    // The physical ENN (Enable) pin is read on IOIN bit 0 (inverted in silicon: 0 = Enabled)
    bool driver_enabled = (ioin & (1U << 0)) == 0;

    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, " FINAL HARDWARE REPORT - AXIS: %s", axis->name);
    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, " UART Address        : 0x%02X", axis->address);
    ESP_LOGI(TAG, " GCONF Register      : 0x%08lX", gconf);
    ESP_LOGI(TAG, " CHOPCONF Register   : 0x%08lX", chopconf);
    ESP_LOGI(TAG, " IOIN Register       : 0x%08lX", ioin);
    ESP_LOGI(TAG, " Microstep Control   : %s", mstep_by_uart ? "DIGITAL (UART)" : "PHYSICAL (MS1/MS2 pins)");
    ESP_LOGI(TAG, " MRES Configuration  : %u (Microsteps: %u)", mres, msteps);
    ESP_LOGI(TAG, " Current Control     : %s",
             current_by_vref ? "ANALOG (VREF potentiometer)" : "DIGITAL (UART register)");
    ESP_LOGI(TAG, " Driver State        : %s",
             driver_enabled ? "ENABLED (Pins active)" : "DISABLED (Motor free)");
    ESP_LOGI(TAG, "=============================================");
}


esp_err_t tmc2209_hw_init(void) {
    uart_config_t config = {
        .baud_rate = TMC_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t result = uart_driver_install(TMC_UART_NUM, 512, 512, 0, NULL, 0);
    if (result != ESP_OK) return result;

    result = uart_param_config(TMC_UART_NUM, &config);
    if (result != ESP_OK) return result;

    result = uart_set_pin(TMC_UART_NUM, TMC_UART_TX_GPIO, TMC_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (result != ESP_OK) return result;


    // Initialisation loop for RA and DEC axes
    for (size_t i = 0; i < sizeof(tmc_axes) / sizeof(tmc_axes[0]); i++) {
        result = tmc_init_driver(&tmc_axes[i]);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Error initialising axis %s", tmc_axes[i].name);
        } else {
            // If initialisation succeeded, read and report the current state cleanly
            vTaskDelay(pdMS_TO_TICKS(10));
            tmc_log_current_status(&tmc_axes[i]);
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }
    return ESP_OK;
}
