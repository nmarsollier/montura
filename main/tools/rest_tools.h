#pragma once

#include <esp_http_server.h>

/*
 * Register an HTTP GET route.
 * Logs a warning and ignores duplicate URI registrations.
 */
void rest_register_get(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *));

/*
 * Register an HTTP POST route.
 */
void rest_register_post(httpd_handle_t server, const char *uri,
                        esp_err_t (*handler)(httpd_req_t *));

/*
 * Register an HTTP PUT route.
 */
void rest_register_put(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *));
