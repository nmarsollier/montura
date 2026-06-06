/* Mount - mount_settings_storage.c
 *
 * Purpose: persist mount settings in NVS.
 */
#include "mount_internal.h"
#include "tools/tools.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MOUNT_SETTINGS_STORAGE";
static const char *NVS_NAMESPACE = "mount";

static void mount_settings_storage_set_default_settings(MountSettings *settings) {
    settings->lat = -32.8908f;
    settings->lon = -68.8272f;
    settings->elevation = 750;
}

/*
 * Business use case: load persisted mount configuration.
 *
 * Objective: start with valid operational parameters, using saved values when
 * available and defaults otherwise.
 */
void mount_settings_storage_load(MountSettings *settings) {
    if (settings == NULL) {
        ESP_LOGE(TAG, "Invalid output settings pointer");
        return;
    }

    mount_settings_storage_set_default_settings(settings);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS namespace not found, using default settings");
        return;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace, using defaults: %s", esp_err_to_name(err));
        return;
    }

    uint32_t lat_u32;
    err = nvs_get_u32(nvs_handle, "lat", &lat_u32);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load latitude from NVS, using default: %s", esp_err_to_name(err));
    } else {
        settings->lat = uint32_to_float(lat_u32);
    }

    uint32_t lon_u32;
    err = nvs_get_u32(nvs_handle, "lon", &lon_u32);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load longitude from NVS, using default: %s", esp_err_to_name(err));
    } else {
        settings->lon = uint32_to_float(lon_u32);
    }

    int32_t elevation_i32;
    err = nvs_get_i32(nvs_handle, "elevation", &elevation_i32);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load elevation from NVS, using default: %s", esp_err_to_name(err));
    } else {
        settings->elevation = (int) elevation_i32;
    }

    nvs_close(nvs_handle);
}

/*
 * Business use case: persist the current mount configuration.
 *
 * Objective: keep site settings across restarts without manual reconfiguration.
 */
void mount_settings_storage_save(const MountSettings *settings) {
    if (settings == NULL) {
        ESP_LOGE(TAG, "Invalid settings pointer");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace for writing: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u32(nvs_handle, "lat", float_to_uint32(settings->lat));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save latitude to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_set_u32(nvs_handle, "lon", float_to_uint32(settings->lon));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save longitude to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_set_i32(nvs_handle, "elevation", (int32_t) settings->elevation);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save elevation to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    ESP_LOGI(TAG, "Settings saved to NVS - lat=%.6f lon=%.6f elevation=%d",
             settings->lat, settings->lon, settings->elevation);

    nvs_close(nvs_handle);
}
