/* Runtime - runtime_http.c
 *
 * Purpose: start the shared HTTP server and delegate route registration
 * to the REST API and Alpaca modules.
 */
#include "runtime.h"

#include "esp_http_server.h"
#include "esp_log.h"

#include "rest.h"
#include "rest_alpaca.h"

static const char *TAG = "RUNTIME_HTTP";

/*
 * Business use case: bring up the HTTP surface of the mount.
 *
 * Objective: create a single httpd instance and hand it to both the REST API
 * and Alpaca modules so they can register their routes on the same server.
 */
void runtime_http_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 128;
    config.lru_purge_enable = true;

    esp_err_t result = httpd_start(&server, &config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(result));
        return;
    }

    rest_server_start(server);
    rest_alpaca_server_start(server);

    ESP_LOGI(TAG, "HTTP server started");
}
