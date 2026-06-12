/* Runtime - runtime_setup.c
 *
 * Purpose: initialize the subsystems needed before the runtime loop starts.
 */
#include "runtime.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "buttons.h"
#include "led.h"
#include "motors.h"
#include "mount.h"
#include "network.h"

static const char *TAG = "RUNTIME_SETUP";

/*
 * Business use case: prepare the mount for operation.
 *
 * Objective: bring network, core services, and peripherals online so the
 * mount starts in a usable state.
 */
void setup_init(void) {
    ESP_LOGI(TAG, "Setting up mount");

    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);

    network_start();

    buttons_init();

    led_init();
    led_external_on(0);

    mount_init();

    motors_init();
    
    ESP_LOGI(TAG, "Mount ready");
}
