#include "upload_manager.h"

#include "measurement.h"
#include "wifi_manager.h"
#include "time_manager.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>

#define TAG "upload"

/* ---- CONFIG ---- */
#define BATCH_SIZE 5

#define SUPABASE_URL "https://hzucoiipjnfhnqjxtrgj.supabase.co/rest/v1/measurements"
#define SUPABASE_API_KEY "sb_publishable_47ApWRf7T1esYfIBUWkRGg_VVbAVhp3"

static bool build_json_payload(char *buf, size_t buf_len, uint32_t count)
{
    size_t offset = 0;
    offset += snprintf(buf + offset, buf_len - offset, "[");

    for (uint32_t i = 0; i < count; i++) {
        measurement_t m;
        if (!measurement_get(i, &m)) {
            return false;
        }

        offset += snprintf(
            buf + offset, buf_len - offset,
            "{"
            "\"timestamp_utc\":%lld,"
            "\"temperature_c_x100\":%d,"
            "\"humidity_x100\":%d,"
            "\"pressure_hpa_x100\":%d,"
            "\"altitude_m_x10\":%d"
            "}",
            (long long)m.timestamp_utc,
            m.temperature_c_x100,
            m.humidity_x100,
            (int)m.pressure_hpa_x100,
            (int)m.altitude_m_x10
        );

        if (i < count - 1) {
            offset += snprintf(buf + offset, buf_len - offset, ",");
        }

        if (offset >= buf_len) {
            ESP_LOGE(TAG, "JSON buffer overflow");
            return false;
        }
    }

    snprintf(buf + offset, buf_len - offset, "]");
    return true;
}


bool upload_manager_try_upload_one_batch(void)
{
    /* ---- GATING ---- */
    if (!wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "WiFi not connected, skipping upload");
        return false;
    }

    if (!time_manager_is_time_valid()) {
        ESP_LOGI(TAG, "Time not valid, skipping upload");
        return false;
    }

    int available = measurement_count();
    if (available < BATCH_SIZE) {
        ESP_LOGI(TAG, "Not enough measurements (%d/%d)",
                 available, BATCH_SIZE);
        return false;
    }

    /* ---- BUILD PAYLOAD ---- */
    char json[1024];   // safe size for 5 measurements
    if (!build_json_payload(json, sizeof(json), BATCH_SIZE)) {
        ESP_LOGE(TAG, "Failed to build JSON payload");
        return false;
    }

    ESP_LOGI(TAG, "Uploading batch: %s", json);

    /* ---- HTTP CLIENT CONFIG ---- */
    esp_http_client_config_t config = {
        .url = SUPABASE_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach, // Use built-in CA bundle for server verification
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return false;
    }

    esp_http_client_set_header(client, "apikey", SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Authorization",
                               "Bearer " SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json, strlen(json));

    /* ---- SEND (BLOCKING, SAFE) ---- */
    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP error: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP status = %d", status);

    esp_http_client_cleanup(client);

    /* ---- CONFIRM SUCCESS ---- */
    if (status == 200 || status == 201) {
        ESP_LOGI(TAG, "Upload successful, deleting batch");
        measurement_delete(BATCH_SIZE);
        return true;
    }

    ESP_LOGW(TAG, "Upload failed, keeping data");
    return false;
}