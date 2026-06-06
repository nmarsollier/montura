/* Rest - rest_api_server.c
 *
 * Purpose: register REST API HTTP routes on the shared server.
 */
#include "rest.h"
#include "rest_tools.h"

#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "REST_API_SERVER";

/*
 * Business use case: publish the device's REST surface.
 *
 * Objective: register all REST API routes on the given HTTP server so the UI,
 * bridges, and automation clients share a single integration point.
 */
void rest_server_start(httpd_handle_t server) {
    rest_register_get(server, "/api/status", rest_status_handler);
    /* Serve the embedded SPA at the root path `/`. */
    rest_register_get(server, "/", rest_screen_handler);

    rest_register_post(server, "/api/tracking", rest_tracking_handler);
    rest_register_post(server, "/api/move-axis", rest_move_axis_handler);
    rest_register_post(server, "/api/goto", rest_goto_handler);
    rest_register_post(server, "/api/stop", rest_stop_handler);
    rest_register_post(server, "/api/park", rest_park_handler);
    rest_register_post(server, "/api/home", rest_home_handler);
    rest_register_post(server, "/api/unpark", rest_unpark_handler);
    rest_register_post(server, "/api/sync", rest_sync_handler);
    rest_register_post(server, "/api/settings", rest_settings_handler);
    rest_register_post(server, "/api/wifi-config", rest_wifi_handler);

    ESP_LOGI(TAG, "REST routes registered");
}
