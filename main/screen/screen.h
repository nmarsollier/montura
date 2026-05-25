#pragma once

#include "esp_err.h"
#include <stdint.h>

#define SCREEN_SSD1306_TEXT_LINES 8
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 128

void screen_init(void);

void screen_update(void);

esp_err_t screen_ssd1306_init(void);

void screen_clear(void);

esp_err_t screen_flush(void);

esp_err_t screen_ssd1306_init(void);

typedef struct {
    int width;

    int height;

    const uint8_t *data;
} ScreenBitmap;

void screen_draw_screen_bitmap(

    int x,

    int y,

    const ScreenBitmap *bitmap);

void screen_draw_bitmap(

    int x,

    int y,

    const uint8_t *bitmap,

    int width,

    int height);

extern const ScreenBitmap ICON_WIFI_ON;

extern const ScreenBitmap ICON_WIFI_OFF;

extern const ScreenBitmap ICON_TRACK_OFF;

extern const ScreenBitmap ICON_TRACK_SIDEREAL;

extern const ScreenBitmap ICON_TRACK_LUNAR;

extern const ScreenBitmap ICON_TRACK_SOLAR;

extern const ScreenBitmap ICON_STATUS_READY;

extern const ScreenBitmap ICON_STATUS_SLEWING;

extern const ScreenBitmap ICON_STATUS_ERROR;

extern const ScreenBitmap ICON_RA;

extern const ScreenBitmap ICON_DEC;

extern const ScreenBitmap ICON_MONTURITA;

void screen_ssd1306_draw_glyph_columns(

    int x,

    int y,

    const uint16_t *columns,

    int width,

    int height);

void screen_draw_text_5x10(int x, int y, const char *text);

void screen_draw_text_8x12(int x, int y, const char *text);
