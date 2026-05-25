/* REST - rest_server.c
 *
 * Purpose: initialize and register the device REST routes.
 */
#include "rest.h"

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "REST_SERVER";

static void register_get(httpd_handle_t server, const char *uri, esp_err_t (*handler)(httpd_req_t *request)) {
    httpd_uri_t route = {
        .uri = uri,
        .method = HTTP_GET,
        .handler = handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(server, &route);
}

static void register_post(httpd_handle_t server, const char *uri, esp_err_t (*handler)(httpd_req_t *request)) {
    httpd_uri_t route = {
        .uri = uri,
        .method = HTTP_POST,
        .handler = handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(server, &route);
}

/*
 * Business use case: publish the device's REST surface.
 *
 * Objective: expose a single integration point for UI, bridges, and
 * automation clients.
 */
void rest_server_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    esp_err_t result = httpd_start(&server, &config);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start REST server: %s", esp_err_to_name(result));
        return;
    }

    register_get(server, "/api/status", rest_status_handler);
    /* Serve the embedded SPA at the root path `/`. */
    register_get(server, "/", rest_screen_handler);

    register_post(server, "/api/tracking", rest_tracking_handler);
    register_post(server, "/api/move-axis", rest_move_axis_handler);
    register_post(server, "/api/goto", rest_goto_handler);
    register_post(server, "/api/stop", rest_stop_handler);
    register_post(server, "/api/park", rest_park_handler);
    register_post(server, "/api/home", rest_home_handler);
    register_post(server, "/api/unpark", rest_unpark_handler);
    register_post(server, "/api/sync", rest_sync_handler);
    register_post(server, "/api/settings", rest_settings_handler);
    register_post(server, "/api/wifi-config", rest_wifi_handler);

    ESP_LOGI(TAG, "REST server started");
}
