/* Runtime - runtime_http.c
 *
 * Purpose: bring up the REST and Alpaca HTTP servers, each on its own port.
 */
#include "runtime.h"

#include "esp_log.h"

#include "rest.h"
#include "rest_alpaca.h"

static const char *TAG = "RUNTIME_HTTP";

/*
 * Business use case: bring up the HTTP surface of the mount.
 *
 * Objective: delegate to the REST (port 80) and Alpaca (port 11111)
 * modules, each of which creates and owns its own httpd instance.
 */
void runtime_http_start(void) {
    rest_server_start();
    rest_alpaca_server_start();

    ESP_LOGI(TAG, "HTTP servers started");
}
