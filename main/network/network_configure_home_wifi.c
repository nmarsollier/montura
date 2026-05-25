/* Network - network.c
*
 * Purpose: initialize Wi-Fi for the physical ESP32 board.
 *
 * The device uses ESP-IDF Wi-Fi driver storage for home network credentials.
 * Monturita does not store a separate SSID/password in its own NVS namespace.
 *
 * Boot behavior:
 * - If the ESP-IDF Wi-Fi driver already has a station SSID, try to connect.
 * - If no station SSID exists, start the setup AP.
 * - If station connection fails, start the setup AP as fallback.
 *
 * Runtime behavior:
 * - REST can call network_configure_home_wifi() to set the station credentials.
 * - ESP-IDF persists those credentials internally.
 */
#include "network.h"

#include <stdbool.h>
#include <string.h>

#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

/* wifi_event_group is defined in network.c */
extern EventGroupHandle_t wifi_event_group;

static const char *TAG = "NETWORK_CONFIGURE_HOME_WIFI";

esp_err_t network_configure_home_wifi(const char *ssid, const char *password) {
    if (ssid == NULL || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Cannot configure Wi-Fi: SSID is empty");
        return ESP_ERR_INVALID_ARG;
    }

    if (password == NULL) {
        ESP_LOGE(TAG, "Cannot configure Wi-Fi: password is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t sta_config = {0};
    strncpy((char *) sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid) - 1);
    strncpy((char *) sta_config.sta.password, password, sizeof(sta_config.sta.password) - 1);
    sta_config.sta.threshold.authmode = strlen(password) == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    wifi_mode_t mode = WIFI_MODE_NULL;
    ESP_RETURN_ON_ERROR(esp_wifi_get_mode(&mode), TAG, "Failed to read Wi-Fi mode");

    if (mode == WIFI_MODE_AP) {
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "Failed to switch Wi-Fi to APSTA mode");
    } else if (mode == WIFI_MODE_NULL) {
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to switch Wi-Fi to STA mode");
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta_config), TAG, "Failed to set station Wi-Fi config");

    wifi_retry_count = 0;
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT);

    if (!wifi_started) {
        ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start Wi-Fi");
        wifi_started = true;
    } else {
        esp_wifi_disconnect();
        ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "Failed to connect Wi-Fi");
    }

    ESP_LOGI(TAG, "Home Wi-Fi configured using ESP-IDF Wi-Fi storage");
    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    return ESP_OK;
}
