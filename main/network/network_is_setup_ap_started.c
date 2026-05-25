#include "network.h"

#include <stdbool.h>
#include <string.h>

#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

bool network_is_setup_ap_started(void) {
    return setup_ap_started;
}
