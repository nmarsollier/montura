/* Network - network_sntp.c
 *
 * Purpose: synchronise the system clock via SNTP as soon as WiFi connects.
 *
 * The ESP-IDF SNTP client queries pool.ntp.org on the first successful
 * WiFi connection and periodically thereafter.  The system clock stays
 * in UTC — all local-time / LST conversions happen in the mount
 * coordinates module using the site longitude.
 *
 * This function is called from the IP_EVENT_STA_GOT_IP handler.  Because
 * the event handler runs in the WiFi task, we defer the actual SNTP work
 * to a short-lived background task so we never block the event loop.
 */

#include "network.h"

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "NETWORK_SNTP";

static void sntp_task(void *arg) {
    (void) arg;

    /*
     * Give the network stack (DNS, DHCP) a couple of seconds to fully
     * settle before attempting the first NTP query.
     */
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "Starting SNTP time synchronisation");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.start = true;
    /* server_from_dhcp requires CONFIG_LWIP_DHCP_GET_NTP_SRV in sdkconfig. */
    config.server_from_dhcp = false;

    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SNTP init failed: %s — time sync unavailable",
                 esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    esp_sntp_set_sync_interval(3600000); /* re-sync every hour (ms) */

    /* Poll for up to 15 s for the first sync to complete. */
    bool synced = false;
    for (int retry = 0; retry < 150; retry++) {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            ESP_LOGI(TAG, "SNTP sync completed");
            time_t now;
            time(&now);
            struct tm timeinfo;
            gmtime_r(&now, &timeinfo);
            ESP_LOGI(TAG, "Current UTC: %04d-%02d-%02dT%02d:%02d:%02dZ",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                     timeinfo.tm_mday, timeinfo.tm_hour,
                     timeinfo.tm_min, timeinfo.tm_sec);
            synced = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!synced) {
        ESP_LOGW(TAG, "SNTP sync did not complete after 15 s");
    }

    vTaskDelete(NULL);
}

void network_sntp_start(void) {
    xTaskCreate(sntp_task, "sntp_sync", 3072, NULL, 1, NULL);
}
