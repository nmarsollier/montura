#pragma once

#include "esp_http_server.h"
#include "mount.h"

void rest_server_start(httpd_handle_t server);

esp_err_t rest_status_handler(httpd_req_t * request);
esp_err_t rest_screen_handler(httpd_req_t * request);
esp_err_t rest_tracking_handler(httpd_req_t * request);
esp_err_t rest_goto_handler(httpd_req_t * request);
esp_err_t rest_move_axis_handler(httpd_req_t * request);
esp_err_t rest_stop_handler(httpd_req_t * request);
esp_err_t rest_park_handler(httpd_req_t * request);
esp_err_t rest_unpark_handler(httpd_req_t * request);
esp_err_t rest_sync_handler(httpd_req_t * request);
esp_err_t rest_settings_handler(httpd_req_t * request);
esp_err_t rest_home_handler(httpd_req_t * request);
esp_err_t rest_wifi_handler(httpd_req_t * request);

void rest_send_result(
    httpd_req_t *request,
    MountResult result);
