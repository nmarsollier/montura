#pragma once

#include <stdbool.h>

#include "esp_err.h"

void network_start(void);

void network_sntp_start(void);

extern char network_ip[17];
extern bool setup_ap_started;

#define WIFI_SETUP_AP_SSID "Monturita"
#define WIFI_SETUP_AP_PASSWORD ""
#define WIFI_SETUP_AP_CHANNEL 1
#define WIFI_SETUP_AP_MAX_CONNECTIONS 4

#define WIFI_MAX_RETRY_COUNT 10
#define WIFI_CONNECT_TIMEOUT_MS 15000

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1

extern int wifi_retry_count;
extern bool wifi_started;


/*
 * Configure the ESP-IDF Wi-Fi driver with home network credentials.
 *
 * The credentials are stored by the ESP-IDF Wi-Fi driver using its own
 * persistent storage. Monturita does not keep a separate Wi-Fi config in NVS.
 */
esp_err_t network_configure_home_wifi(const char *ssid, const char *password);

bool network_is_setup_ap_started(void);

char *network_get_ip(void);
