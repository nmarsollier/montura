/* Rest - rest_screen_handler.c
 *
 * Purpose: serve the embedded SPA (index.html) at the root path.
 */
#include "rest.h"

#include "tools/tools.h"

esp_err_t rest_screen_handler(httpd_req_t *request) {
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[]   asm("_binary_index_html_end");
    unsigned int len = index_html_end - index_html_start;
    http_response_html(request, (const char *) index_html_start, len);
    return ESP_OK;
}
