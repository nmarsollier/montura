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


static char *RaHMS_to_String(RaHMS data) {
    static char buf[16];
    sprintf(buf, "%02d:%02d:%05.2f", data.hours, data.minutes, data.seconds);
    return buf;
}

static char *DecDMS_to_String(DecDMS data) {
    static char buf[16];
    sprintf(buf, "%02d:%02d:%05.2f", data.degrees, data.minutes, data.seconds);
    return buf;
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

static const char *string_capitalize(const char *text) {
    static char buffer[8];
    bool new_word = true;
    size_t index = 0;

    while (*text && index < sizeof(buffer) - 1) {
        unsigned char character = (unsigned char) *text;
        if (isspace(character)) {
            new_word = true;
            buffer[index++] = (char) character;
        } else {
            buffer[index++] = (char) (
                new_word
                    ? toupper(character)
                    : tolower(character));
            new_word = false;
        }
        text++;
    }
    buffer[index] = '\0';
    return buffer;
}

static const ScreenBitmap *tracking_mode_icon(TrackingMode mode) {
    switch (mode) {
        case TRACKING_SIDEREAL:
            return &ICON_TRACK_SIDEREAL;

        case TRACKING_LUNAR:
            return &ICON_TRACK_LUNAR;

        case TRACKING_SOLAR:
            return &ICON_TRACK_SOLAR;

        case TRACKING_NONE:
        case TRACKING_MANUAL:
        case TRACKING_UNKNOWN:
        default:
            return &ICON_TRACK_OFF;
    }
}

static const ScreenBitmap *motors_status_icon(MotorsStatus status) {
    switch (status) {
        case MOUNT_STATUS_READY:
            return &ICON_STATUS_READY;

        case MOUNT_STATUS_SLEWING:
            return &ICON_STATUS_SLEWING;

        case MOUNT_STATUS_TRACKING:
            return &ICON_TRACK_SIDEREAL;

        case MOUNT_STATUS_PARKED:
        case MOUNT_STATUS_DISABLED:
        case MOUNT_STATUS_UNKNOWN:
        default:
            return &ICON_STATUS_ERROR;
    }
}

void screen_update(void) {
    static int64_t last_state_update = -1;
    VisibleStatusData data = mount_get_visible_status_data();

    if (data.last_update <= last_state_update) {
        return;
    }

    last_state_update = data.last_update;

    if (!screen_ready) return;

    screen_clear();

    int pos = 4;
    screen_draw_screen_bitmap(0, pos, &ICON_MONTURITA);
    screen_draw_text_8x12(16, pos, "ontu");
    pos += 14;
    screen_draw_text_8x12(16, pos, "rita!");

    /* WIFI */

    pos += 16;
    screen_draw_screen_bitmap(0, pos, &ICON_WIFI_ON);
    screen_draw_text_8x12(16, pos + 1, string_last_5(network_get_ip()));

    /* Tracking */
    pos += 18;
    screen_draw_screen_bitmap(0, pos, tracking_mode_icon(data.tracking));
    screen_draw_text_5x10(16, pos + 2, string_capitalize(string_first_7(tracking_to_string(data.tracking))));

    /* Status */
    pos += 18;
    screen_draw_screen_bitmap(0, pos, motors_status_icon(data.status));
    screen_draw_text_5x10(16, pos + 2, string_capitalize(string_first_7(status_to_string(data.status))));

    /* RA */
    pos += 18;
    screen_draw_screen_bitmap(0, pos, &ICON_RA);
    screen_draw_text_5x10(16, pos + 2, RaHMS_to_String(data.ra));

    /* DEC */
    pos += 18;
    screen_draw_screen_bitmap(0, pos, &ICON_DEC);
    screen_draw_text_5x10(16, pos + 2, DecDMS_to_String(data.dec));

    screen_flush();
}

