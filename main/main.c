/* Main - main.c
 *
 * Purpose: start the runtime and REST server from `app_main`.
 */
#include "esp_log.h"

#include "rest.h"
#include "runtime.h"

static const char *TAG = "MAIN";

void app_main(void) {
    setup_init();

    setup_runtime_start();

    rest_server_start();

    ESP_LOGI(TAG, "Started");
}
