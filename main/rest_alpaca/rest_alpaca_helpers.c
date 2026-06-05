#include "rest_alpaca_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "esp_http_server.h"
#include "esp_log.h"

#define ALPACA_RESPONSE_BUFFER 512

static uint32_t s_server_tx = 0;

uint32_t alpaca_next_server_tx(void) {
    return ++s_server_tx;
}

/* ─── Internal helper ─── */

static void alpaca_send_json(httpd_req_t *req, const char *json) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
}

/* ─── Public API ─── */

void alpaca_response_value(httpd_req_t *req, const char *value_json,
                           uint32_t client_id, uint32_t server_tx) {
    char buf[ALPACA_RESPONSE_BUFFER];
    snprintf(buf, sizeof(buf),
             "{\"Value\":%s,\"ClientTransactionID\":%lu,"
             "\"ServerTransactionID\":%lu,\"ErrorNumber\":0,"
             "\"ErrorMessage\":\"\"}",
             value_json, client_id, server_tx);
    alpaca_send_json(req, buf);
}

void alpaca_response_ok(httpd_req_t *req,
                        uint32_t client_id, uint32_t server_tx) {
    char buf[ALPACA_RESPONSE_BUFFER];
    snprintf(buf, sizeof(buf),
             "{\"ClientTransactionID\":%lu,"
             "\"ServerTransactionID\":%lu,\"ErrorNumber\":0,"
             "\"ErrorMessage\":\"\"}",
             client_id, server_tx);
    alpaca_send_json(req, buf);
}

void alpaca_response_error(httpd_req_t *req, int error_number,
                           const char *message,
                           uint32_t client_id, uint32_t server_tx) {
    char buf[ALPACA_RESPONSE_BUFFER];
    snprintf(buf, sizeof(buf),
             "{\"ClientTransactionID\":%lu,"
             "\"ServerTransactionID\":%lu,"
             "\"ErrorNumber\":%d,"
             "\"ErrorMessage\":\"%s\"}",
             client_id, server_tx, error_number, message);
    alpaca_send_json(req, buf);
}

/* ─── Parameter parsing ─── */

uint32_t alpaca_get_client_id(httpd_req_t *req) {
    char buf[32];
    esp_err_t err = httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (err != ESP_OK) return 0;

    char value[16];
    err = httpd_query_key_value(buf, "ClientID", value, sizeof(value));
    if (err != ESP_OK) {
        /* Also try lowercase */
        err = httpd_query_key_value(buf, "clientid", value, sizeof(value));
        if (err != ESP_OK) return 0;
    }
    return (uint32_t) atol(value);
}

char *alpaca_get_form_param(httpd_req_t *req, const char *key) {
    char *body = malloc(512);
    if (!body) return NULL;

    int ret = httpd_req_recv(req, body, 511);
    if (ret <= 0) {
        free(body);
        return NULL;
    }
    body[ret] = '\0';

    /* Parse key=value from URL-encoded body.
       Simple parser: find key, then extract value until & or end. */
    size_t key_len = strlen(key);
    char *p = body;
    while (*p) {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            p += key_len + 1;
            char *val_end = p;
            while (*val_end && *val_end != '&') val_end++;
            size_t vlen = val_end - p;
            char *value = malloc(vlen + 1);
            if (value) {
                memcpy(value, p, vlen);
                value[vlen] = '\0';
            }
            free(body);
            return value;
        }
        /* Skip to next & */
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }

    free(body);
    return NULL;
}

bool alpaca_get_form_float(httpd_req_t *req, const char *key, float *out) {
    char *val = alpaca_get_form_param(req, key);
    if (!val) return false;
    *out = (float) atof(val);
    free(val);
    return true;
}

bool alpaca_get_form_bool(httpd_req_t *req, const char *key, bool *out) {
    char *val = alpaca_get_form_param(req, key);
    if (!val) return false;
    if (strcasecmp(val, "true") == 0 || strcmp(val, "1") == 0)
        *out = true;
    else
        *out = false;
    free(val);
    return true;
}

bool alpaca_get_form_int(httpd_req_t *req, const char *key, int *out) {
    char *val = alpaca_get_form_param(req, key);
    if (!val) return false;
    *out = atoi(val);
    free(val);
    return true;
}
