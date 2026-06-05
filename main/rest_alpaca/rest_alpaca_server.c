/* rest_alpaca_server.c — Alpaca HTTP server startup and route registration */

#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "rest_tools.h"

#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "REST_ALPACA_SERVER";

static httpd_handle_t s_alpaca_server = NULL;

#define T  "/api/v1/telescope/0"

/* ─── Server startup ─── */

esp_err_t rest_alpaca_server_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = ALPACA_DEFAULT_PORT;
    config.max_uri_handlers = ALPACA_MAX_URI_HANDLERS;
    config.lru_purge_enable = true;

    esp_err_t result = httpd_start(&s_alpaca_server, &config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Alpaca server: %s", esp_err_to_name(result));
        return result;
    }

    /* ─── Management API ─── */
    rest_register_get(s_alpaca_server, "/management/v1/apiversions",
                      alpaca_management_apiversions_handler);
    rest_register_get(s_alpaca_server, "/management/v1/description",
                      alpaca_management_description_handler);
    rest_register_get(s_alpaca_server, "/management/v1/configureddevices",
                      alpaca_management_configureddevices_handler);

    /* ─── Common device endpoints ─── */
    rest_register_get(s_alpaca_server, T "/connected", alpaca_connected_handler);
    rest_register_put(s_alpaca_server, T "/connected", alpaca_connected_put_handler);
    rest_register_get(s_alpaca_server, T "/description", alpaca_description_handler);
    rest_register_get(s_alpaca_server, T "/driverinfo", alpaca_driverinfo_handler);
    rest_register_get(s_alpaca_server, T "/driverversion", alpaca_driverversion_handler);
    rest_register_get(s_alpaca_server, T "/interfaceversion", alpaca_interfaceversion_handler);
    rest_register_get(s_alpaca_server, T "/name", alpaca_name_handler);
    rest_register_get(s_alpaca_server, T "/supportedactions", alpaca_supportedactions_handler);

    /* ─── Capabilities ─── */
    rest_register_get(s_alpaca_server, T "/canfindhome", alpaca_canfindhome_handler);
    rest_register_get(s_alpaca_server, T "/canpark", alpaca_canpark_handler);
    rest_register_get(s_alpaca_server, T "/canpulseguide", alpaca_canpulseguide_handler);
    rest_register_get(s_alpaca_server, T "/cansetdeclinationrate", alpaca_cansetdeclinationrate_handler);
    rest_register_get(s_alpaca_server, T "/cansetguiderates", alpaca_cansetguiderates_handler);
    rest_register_get(s_alpaca_server, T "/cansetpark", alpaca_cansetpark_handler);
    rest_register_get(s_alpaca_server, T "/cansetpierside", alpaca_cansetpierside_handler);
    rest_register_get(s_alpaca_server, T "/cansetrightascensionrate", alpaca_cansetrightascensionrate_handler);
    rest_register_get(s_alpaca_server, T "/cansettracking", alpaca_cansettracking_handler);
    rest_register_get(s_alpaca_server, T "/canslew", alpaca_canslew_handler);
    rest_register_get(s_alpaca_server, T "/canslewaltaz", alpaca_canslewaltaz_handler);
    rest_register_get(s_alpaca_server, T "/canslewaltazasync", alpaca_canslewaltazasync_handler);
    rest_register_get(s_alpaca_server, T "/canslewasync", alpaca_canslewasync_handler);
    rest_register_get(s_alpaca_server, T "/cansync", alpaca_cansync_handler);
    rest_register_get(s_alpaca_server, T "/cansyncaltaz", alpaca_cansyncaltaz_handler);
    rest_register_get(s_alpaca_server, T "/canunpark", alpaca_canunpark_handler);
    rest_register_get(s_alpaca_server, T "/canmoveaxis", alpaca_canmoveaxis_handler);

    /* ─── Telescope properties GET ─── */
    rest_register_get(s_alpaca_server, T "/alignmentmode", alpaca_alignmentmode_handler);
    rest_register_get(s_alpaca_server, T "/altitude", alpaca_altitude_handler);
    rest_register_get(s_alpaca_server, T "/aperturearea", alpaca_aperturearea_handler);
    rest_register_get(s_alpaca_server, T "/aperturediameter", alpaca_aperturediameter_handler);
    rest_register_get(s_alpaca_server, T "/athome", alpaca_athome_handler);
    rest_register_get(s_alpaca_server, T "/atpark", alpaca_atpark_handler);
    rest_register_get(s_alpaca_server, T "/azimuth", alpaca_azimuth_handler);
    rest_register_get(s_alpaca_server, T "/declination", alpaca_declination_handler);
    rest_register_get(s_alpaca_server, T "/declinationrate", alpaca_declinationrate_handler);
    rest_register_get(s_alpaca_server, T "/equatorialsystem", alpaca_equatorialsystem_handler);
    rest_register_get(s_alpaca_server, T "/focallength", alpaca_focallength_handler);
    rest_register_get(s_alpaca_server, T "/guideratedeclination", alpaca_guideratedec_handler);
    rest_register_get(s_alpaca_server, T "/guideraterightascension", alpaca_guideratera_handler);
    rest_register_get(s_alpaca_server, T "/ispulseguiding", alpaca_ispulseguiding_handler);
    rest_register_get(s_alpaca_server, T "/rightascension", alpaca_rightascension_handler);
    rest_register_get(s_alpaca_server, T "/rightascensionrate", alpaca_rightascensionrate_handler);
    rest_register_get(s_alpaca_server, T "/sideofpier", alpaca_sideofpier_handler);
    rest_register_get(s_alpaca_server, T "/siderealtime", alpaca_siderealtime_handler);
    rest_register_get(s_alpaca_server, T "/siteelevation", alpaca_siteelevation_handler);
    rest_register_get(s_alpaca_server, T "/sitelatitude", alpaca_sitelatitude_handler);
    rest_register_get(s_alpaca_server, T "/sitelongitude", alpaca_sitelongitude_handler);
    rest_register_get(s_alpaca_server, T "/slewing", alpaca_slewing_handler);
    rest_register_get(s_alpaca_server, T "/slewsettletime", alpaca_slewsettletime_handler);
    rest_register_get(s_alpaca_server, T "/targetdeclination", alpaca_targetdec_handler);
    rest_register_get(s_alpaca_server, T "/targetrightascension", alpaca_targetra_handler);
    rest_register_get(s_alpaca_server, T "/tracking", alpaca_tracking_handler);
    rest_register_get(s_alpaca_server, T "/trackingrate", alpaca_trackingrate_handler);
    rest_register_get(s_alpaca_server, T "/trackingrates", alpaca_trackingrates_handler);
    rest_register_get(s_alpaca_server, T "/utcdate", alpaca_utcdate_handler);

    /* Mock endpoints */
    rest_register_get(s_alpaca_server, T "/axisrates", alpaca_axisrates_handler);
    rest_register_get(s_alpaca_server, T "/doesrefraction", alpaca_doesrefraction_handler);
    rest_register_get(s_alpaca_server, T "/destinationsideofpier", alpaca_destinationsideofpier_handler);

    /* ─── Telescope methods PUT ─── */
    rest_register_put(s_alpaca_server, T "/abortslew", alpaca_abortslew_handler);
    rest_register_put(s_alpaca_server, T "/findhome", alpaca_findhome_handler);
    rest_register_put(s_alpaca_server, T "/park", alpaca_park_handler);
    rest_register_put(s_alpaca_server, T "/unpark", alpaca_unpark_handler);
    rest_register_put(s_alpaca_server, T "/setpark", alpaca_setpark_handler);
    rest_register_put(s_alpaca_server, T "/slewtocoordinatesasync", alpaca_slewtocoordinatesasync_handler);
    rest_register_put(s_alpaca_server, T "/slewtocoordinates", alpaca_slewtocoordinatesasync_handler);
    rest_register_put(s_alpaca_server, T "/slewtoaltazasync", alpaca_slewtoaltazasync_handler);
    rest_register_put(s_alpaca_server, T "/synctocoordinates", alpaca_synctocoordinates_handler);
    rest_register_put(s_alpaca_server, T "/synctoaltaz", alpaca_synctoaltaz_handler);
    rest_register_put(s_alpaca_server, T "/slewtotargetasync", alpaca_slewtotargetasync_handler);
    rest_register_put(s_alpaca_server, T "/slewtotarget", alpaca_slewtotargetasync_handler);
    rest_register_put(s_alpaca_server, T "/synctotarget", alpaca_synctotarget_handler);
    rest_register_put(s_alpaca_server, T "/pulseguide", alpaca_pulseguide_handler);
    rest_register_put(s_alpaca_server, T "/moveaxis", alpaca_moveaxis_handler);

    /* Property setters */
    rest_register_put(s_alpaca_server, T "/tracking", alpaca_tracking_put_handler);
    rest_register_put(s_alpaca_server, T "/trackingrate", alpaca_trackingrate_put_handler);
    rest_register_put(s_alpaca_server, T "/utcdate", alpaca_utcdate_put_handler);
    rest_register_put(s_alpaca_server, T "/sitelatitude", alpaca_sitelatitude_put_handler);
    rest_register_put(s_alpaca_server, T "/sitelongitude", alpaca_sitelongitude_put_handler);
    rest_register_put(s_alpaca_server, T "/siteelevation", alpaca_siteelevation_put_handler);
    rest_register_put(s_alpaca_server, T "/targetrightascension", alpaca_targetra_put_handler);
    rest_register_put(s_alpaca_server, T "/targetdeclination", alpaca_targetdec_put_handler);
    rest_register_put(s_alpaca_server, T "/sideofpier", alpaca_sideofpier_put_handler);
    rest_register_put(s_alpaca_server, T "/slewsettletime", alpaca_slewsettletime_put_handler);
    rest_register_put(s_alpaca_server, T "/declinationrate", alpaca_declinationrate_put_handler);
    rest_register_put(s_alpaca_server, T "/rightascensionrate", alpaca_rightascensionrate_put_handler);
    rest_register_put(s_alpaca_server, T "/guideratedeclination", alpaca_guideratedec_put_handler);
    rest_register_put(s_alpaca_server, T "/guideraterightascension", alpaca_guideratera_put_handler);

    ESP_LOGI(TAG, "Alpaca server started on port %d", ALPACA_DEFAULT_PORT);
    return ESP_OK;
}
