/* Tools - rest_tools.c
 *
 * Shared HTTP route registration helpers used by both the REST API and
 * Alpaca servers. Avoids duplicating register_get / register_post / register_put
 * across server modules.
 */
#include "rest_tools.h"

#include <esp_log.h>

static const char *TAG = "REST_TOOLS";

void rest_register_get(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *)) {
    httpd_uri_t route = {
        .uri      = uri,
        .method   = HTTP_GET,
        .handler  = handler,
        .user_ctx = NULL,
    };
    esp_err_t err = httpd_register_uri_handler(server, &route);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        ESP_LOGW(TAG, "GET  %s: %s", uri, esp_err_to_name(err));
    }
}

void rest_register_post(httpd_handle_t server, const char *uri,
                        esp_err_t (*handler)(httpd_req_t *)) {
    httpd_uri_t route = {
        .uri      = uri,
        .method   = HTTP_POST,
        .handler  = handler,
        .user_ctx = NULL,
    };
    esp_err_t err = httpd_register_uri_handler(server, &route);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        ESP_LOGW(TAG, "POST %s: %s", uri, esp_err_to_name(err));
    }
}

void rest_register_put(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *)) {
    httpd_uri_t route = {
        .uri      = uri,
        .method   = HTTP_PUT,
        .handler  = handler,
        .user_ctx = NULL,
    };
    esp_err_t err = httpd_register_uri_handler(server, &route);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        ESP_LOGW(TAG, "PUT  %s: %s", uri, esp_err_to_name(err));
    }
}
