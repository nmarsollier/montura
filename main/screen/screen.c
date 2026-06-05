#include "screen.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "mount.h"
#include <stdbool.h>

#include "network.h"
#include "tools.h"

#define SCREEN_BUFFER_SIZE 256

static const char *TAG = "SCREEN";

static bool screen_ready = false;

void screen_init(void) {
    esp_err_t result = screen_ssd1306_init();

    if (result != ESP_OK) {
        screen_ready = false;
        ESP_LOGW(TAG, "OLED initialization failed: %s", esp_err_to_name(result));
        return;
    }

    screen_ready = true;

    screen_clear();
    screen_draw_text_5x10(0, 0, "Monturita");
    screen_draw_text_5x10(0, 16, "OLED READY");
    screen_flush();

    ESP_LOGI(TAG, "OLED initialized");
}

/** Non thread safe screen only quick string tool*/
const char *string_last_7(const char *text) {
    static char buffer[8];

    if (text == NULL) {
        buffer[0] = '\0';
        return buffer;
    }

    size_t length = strlen(text);

    const char *start =
            length > 7
                ? text + (length - 7)
                : text;

    strncpy(buffer, start, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    return buffer;
}

const char *string_last_5(const char *text) {
    static char buffer[6];

    if (text == NULL) {
        buffer[0] = '\0';
        return buffer;
    }

    size_t length = strlen(text);

    const char *start =
            length > 5
                ? text + (length - 5)
                : text;

    strncpy(buffer, start, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    return buffer;
}

const char *string_first_7(const char *text) {
    static char buffer[8];
    if (text == NULL) {
        buffer[0] = '\0';
        return buffer;
    }
    snprintf(buffer, sizeof(buffer), "%.7s", text);
    return buffer;
}


/* ─── Big status icon selection by priority ─── */

static const ScreenBitmap *big_status_icon(VisibleStatusData data, bool wifi_ok) {
    if (!wifi_ok) {
        return &BIG_WIFI_OFF;
    }

    if (data.tracking == TRACKING_MANUAL) {
        return &BIG_ALERT;
    }

    if (data.status == MOUNT_STATUS_PARKED) {
        return &BIG_PARKED;
    }

    if (data.status == MOUNT_STATUS_SLEWING) {
        return &BIG_SLEWING;
    }

    if (data.tracking == TRACKING_SIDEREAL) {
        return &BIG_STAR;
    }
    if (data.tracking == TRACKING_LUNAR) {
        return &BIG_MOON;
    }
    if (data.tracking == TRACKING_SOLAR) {
        return &BIG_SUN;
    }

    if (data.status == MOUNT_STATUS_READY) {
        return &BIG_READY;
    }

    return &BIG_STOP;
}

/* ─── Layout constants ─── */

#define YELLOW_BAND_WIDTH  16   /* Left yellow zone */
#define BLUE_MARGIN_RIGHT   8   /* Right margin in blue zone */
#define BIG_ICON_X         16   /* Start of blue zone */
#define BIG_ICON_MAX_W      40  /* Max icon width (64 - 16 - 8) */

void screen_update(void) {
    static int64_t last_state_update = -1;
    VisibleStatusData data = mount_get_visible_status_data();

    if (data.last_update <= last_state_update) {
        return;
    }

    last_state_update = data.last_update;

    if (!screen_ready) return;

    screen_clear();

    /* ─── Top section: Logo + WiFi (yellow band) ─── */

    int pos = 4;
    screen_draw_screen_bitmap(0, pos, &ICON_MONTURITA);
    screen_draw_text_8x12(16, pos, "ontu");
    pos += 14;
    screen_draw_text_8x12(16, pos, " rita");

    /* WiFi status */
    bool wifi_ok = !network_is_setup_ap_started();

    pos += 16;
    screen_draw_screen_bitmap(0, pos, wifi_ok ? &ICON_WIFI_ON : &ICON_WIFI_OFF);
    screen_draw_text_8x12(16, pos + 1, string_last_5(network_get_ip()));

    /* ─── Blue zone: big status icon ─── */

    const ScreenBitmap *big_icon = big_status_icon(data, wifi_ok);

    /* Center the icon vertically in the remaining space.
       Available: pos + 18 to 128. Icon is big_icon->height pixels tall. */
    int remaining_top = pos + 18;
    int remaining_h = SCREEN_HEIGHT - remaining_top;

    int icon_y = remaining_top;
    if (big_icon->height < remaining_h) {
        icon_y = remaining_top + (remaining_h - big_icon->height) / 2;
    }

    /* Center the icon horizontally in the blue zone.
       Blue zone: x=16 to x=63 → 48px wide. Icon is big_icon->width px wide. */
    int blue_w = SCREEN_WIDTH - YELLOW_BAND_WIDTH; /* 48 */
    int icon_x = YELLOW_BAND_WIDTH + (blue_w - big_icon->width) / 2;

    screen_draw_screen_bitmap(icon_x, icon_y, big_icon);

    screen_flush();
}
