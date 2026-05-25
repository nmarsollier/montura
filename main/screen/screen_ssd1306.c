#include "screen.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/gpio_types.h"

#define OLED_WIDTH 128
#define OLED_PAGES 8
#define OLED_BUFFER_SIZE (OLED_WIDTH * OLED_PAGES)
/* Physical OLED is 128x64; logical buffer is portrait (64x128) so we will map accordingly */

#define OLED_I2C_ADDRESS 0x3C
#define OLED_I2C_SDA GPIO_NUM_21
#define OLED_I2C_SCL GPIO_NUM_22
#define OLED_I2C_FREQUENCY_HZ 400000

#define OLED_CONTROL_COMMAND 0x00
#define OLED_CONTROL_DATA 0x40

static const char *TAG = "SCREEN_SSD1306";

/* Forward declarations for functions used before their definitions */
void screen_ssd1306_clear(void);

esp_err_t screen_ssd1306_flush(void);

static void logical_clear(void);

static void logical_to_physical(void);

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t i2c_device = NULL;
static uint8_t framebuffer[OLED_BUFFER_SIZE];
static bool initialized = false;

/* Logical framebuffer: one bit per pixel, indexed as logical_buffer[x + y*SCREEN_WIDTH]
   where each byte holds 8 vertical pixels (for convenience we store as bytes per pixel here)
   but representation chosen as 1 byte per pixel (0 or 1) for simplicity. */
static uint8_t logical_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];

static void logical_clear(void) {
    memset(logical_buffer, 0, sizeof(logical_buffer));
}

static void logical_set_pixel(int x, int y, int value) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    logical_buffer[y * SCREEN_WIDTH + x] = value ? 1 : 0;
}

static int logical_get_pixel(int x, int y) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return 0;
    return logical_buffer[y * SCREEN_WIDTH + x];
}

#define FONT_WIDTH 5
#define FONT_HEIGHT 10
#define FONT_SPACING 1
#define FONT_ADVANCE (FONT_WIDTH + FONT_SPACING)
#define FONT_LINE_HEIGHT 11

static esp_err_t oled_write(const uint8_t *data, size_t size) {
    return i2c_master_transmit(i2c_device, data, size, -1);
}

static esp_err_t oled_command(uint8_t command) {
    uint8_t payload[] = {OLED_CONTROL_COMMAND, command};
    return oled_write(payload, sizeof(payload));
}

static esp_err_t oled_command_list(const uint8_t *commands, size_t size) {
    for (size_t i = 0; i < size; i++) {
        esp_err_t result = oled_command(commands[i]);

        if (result != ESP_OK) {
            return result;
        }
    }

    return ESP_OK;
}

static esp_err_t oled_data(const uint8_t *data, size_t size) {
    uint8_t payload[17];

    payload[0] = OLED_CONTROL_DATA;

    while (size > 0) {
        size_t chunk = size > 16 ? 16 : size;

        memcpy(&payload[1], data, chunk);

        esp_err_t result = oled_write(payload, chunk + 1);

        if (result != ESP_OK) {
            return result;
        }

        data += chunk;
        size -= chunk;
    }

    return ESP_OK;
}

esp_err_t screen_ssd1306_init(void) {
    if (initialized) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_I2C_SDA,
        .scl_io_num = OLED_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t result = i2c_new_master_bus(&bus_config, &i2c_bus);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(result));
        return result;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_I2C_ADDRESS,
        .scl_speed_hz = OLED_I2C_FREQUENCY_HZ,
    };

    result = i2c_master_bus_add_device(i2c_bus, &device_config, &i2c_device);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SSD1306 device: %s", esp_err_to_name(result));
        return result;
    }

    const uint8_t init_commands[] = {
        0xAE,
        0xD5, 0x80,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0x8D, 0x14,
        0x20, 0x00,
        0xA1, 0xC8, // Screen layout
        0xDA, 0x12,
        0x81, 0xCF,
        0xD9, 0xF1,
        0xDB, 0x40,
        0xA4,
        0xA6,
        0x2E,
        0xAF,
    };

    result = oled_command_list(init_commands, sizeof(init_commands));

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SSD1306: %s", esp_err_to_name(result));
        return result;
    }

    initialized = true;
    screen_ssd1306_clear();

    return screen_ssd1306_flush();
}

void screen_ssd1306_clear(void) {
    /* Clear both logical and physical buffers */
    logical_clear();
    memset(framebuffer, 0, sizeof(framebuffer));
}

/* Convert logical_buffer (pixels) into framebuffer bytes organized by pages
   SSD1306 expects pages of 8 vertical pixels per byte. */
static void logical_to_physical(void) {
    /* We need to rotate logical buffer (SCREEN_WIDTH x SCREEN_HEIGHT) 90° CW
       into physical framebuffer (OLED_WIDTH x SCREEN_HEIGHT_phys=64).
       Logical is 64x128 (width x height). Physical is 128x64 organized in pages of 8 vertical pixels.

       Mapping: logical (lx,ly) where 0<=lx<64, 0<=ly<128.
       After 90deg CW rotation, pixel moves to physical coordinates:
         px = ly (0..127)
         py = SCREEN_WIDTH - 1 - lx (0..63)
       Then map (px,py) into framebuffer page/column format: page = py / 8, bit = py % 8, column = px
    */
    memset(framebuffer, 0, sizeof(framebuffer));

    for (int lx = 0; lx < SCREEN_WIDTH; lx++) {
        for (int ly = 0; ly < SCREEN_HEIGHT; ly++) {
            if (!logical_get_pixel(lx, ly)) continue;

            /* CCW (-90°) rotation mapping:
               logical (lx,ly) -> physical (px,py)
               px = SCREEN_HEIGHT - 1 - ly  (0..127)
               py = lx                       (0..63)
            */
            int px = SCREEN_HEIGHT - 1 - ly; /* 0..127 */
            int py = lx; /* 0..63 */

            int page = py / 8;
            int bit = py % 8;

            framebuffer[page * OLED_WIDTH + px] |= (1 << bit);
        }
    }
}

void screen_ssd1306_draw_glyph_columns(
    int x,
    int y,
    const uint16_t *columns,
    int width,
    int height) {
    if (columns == NULL || width <= 0 || height <= 0) {
        return;
    }

    /* Skip only when the glyph is completely outside the logical screen. */
    if (x + width <= 0 || x >= SCREEN_WIDTH || y + height <= 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    for (int column = 0; column < width; column++) {
        int target_x = x + column;

        if (target_x < 0) {
            continue;
        }

        if (target_x >= SCREEN_WIDTH) {
            break;
        }

        uint16_t column_data = columns[column];

        for (int bit = 0; bit < height; bit++) {
            int target_y = y + bit;

            if (target_y < 0) {
                continue;
            }

            if (target_y >= SCREEN_HEIGHT) {
                break;
            }

            int pixel = (column_data >> bit) & 0x01;
            logical_set_pixel(target_x, target_y, pixel);
        }
    }
}

esp_err_t screen_flush(void) {
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Convert logical -> physical framebuffer */
    logical_to_physical();

    const uint8_t commands[] = {
        0x21, 0x00, 0x7F,
        0x22, 0x00, 0x07,
    };

    esp_err_t result = oled_command_list(commands, sizeof(commands));

    if (result != ESP_OK) {
        return result;
    }

    return oled_data(framebuffer, sizeof(framebuffer));
}

/* Backwards-compat wrapper */
esp_err_t screen_ssd1306_flush(void) {
    return screen_flush();
}

/* Public API: clear logical buffer (exposed in screen.h as screen_clear) */
void screen_clear(void) {
    logical_clear();
}

void screen_draw_screen_bitmap(
    int x,
    int y,
    const ScreenBitmap *bitmap) {
    if (bitmap == NULL || bitmap->data == NULL || bitmap->width <= 0 || bitmap->height <= 0) {
        return;
    }

    int bytes_per_row = (bitmap->width + 7) / 8;

    for (int row = 0; row < bitmap->height; row++) {
        for (int col = 0; col < bitmap->width; col++) {
            int byte_index = row * bytes_per_row + (col / 8);
            int bit_index = 7 - (col % 8);

            bool enabled = (bitmap->data[byte_index] & (1 << bit_index)) != 0;

            logical_set_pixel(x + col, y + row, enabled);
        }
    }
}

/* Legacy wrapper. */
void screen_draw_bitmap(
    int x,
    int y,
    const uint8_t *bitmap,
    int width,
    int height) {
    ScreenBitmap screen_bitmap = {
        .width = width,
        .height = height,
        .data = bitmap,
    };

    screen_draw_screen_bitmap(x, y, &screen_bitmap);
}
