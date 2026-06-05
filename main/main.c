/* Main - main.c
 *
 * Purpose: start the runtime, REST API server, and Alpaca server.
 */
#include "esp_log.h"

#include "rest.h"
#include "rest_alpaca.h"
#include "runtime.h"

static const char *TAG = "MAIN";

void app_main(void) {
    setup_init();

    setup_runtime_start();

    rest_server_start();

    rest_alpaca_server_start();

    ESP_LOGI(TAG, "Started");
}
