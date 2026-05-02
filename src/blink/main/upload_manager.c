#include "upload_manager.h"

#include "measurement.h"
#include "wifi_manager.h"
#include "time_manager.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>

// For JSON construction (optional, can build manually if preferred)
#include "cJSON.h"

#define TAG "upload"

/* ---- CONFIG ---- */
#define BATCH_SIZE 5

#define SUPABASE_URL "https://hzucoiipjnfhnqjxtrgj.supabase.co/rest/v1/measurements"
#define SUPABASE_API_KEY "sb_publishable_47ApWRf7T1esYfIBUWkRGg_VVbAVhp3"

#define STATION_ID 1   // Must exist in your stations table

static bool build_json_payload(char *buf, size_t buf_len, uint32_t count)
{
    // size_t offset = 0;
    // offset += snprintf(buf + offset, buf_len - offset, "[");

    cJSON *root = cJSON_CreateArray();

    for (uint32_t i = 0; i < count; i++) {
        measurement_t m;
        if (!measurement_get(i, &m)) {
            return false;
        }
        
        /* Convert timestamp to ISO 8601 string for better readability in Supabase (optional) */
        char ts_buf[25];
        struct tm tm_info;
        time_t t = (time_t)m.timestamp_utc;
        gmtime_r(&t, &tm_info);

        strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%dT%H:%M:%SZ", &tm_info);

        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "station_id", STATION_ID);
        cJSON_AddNumberToObject(obj, "temperature_c_x100", m.temperature_c_x100);
        cJSON_AddNumberToObject(obj, "humidity_x100", m.humidity_x100);
        cJSON_AddNumberToObject(obj, "pressure_hpa_x100", (int)m.pressure_hpa_x100);
        cJSON_AddNumberToObject(obj, "altitude_m_x10", (int)m.altitude_m_x10);
        cJSON_AddStringToObject(obj, "measured_at", ts_buf);
        cJSON_AddItemToArray(root, obj);
    }

    char *json_string = cJSON_Print(root);
    if (!json_string) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        return false;
    }

    snprintf(buf, buf_len, "%s", json_string);
    // buf = cJSON_PrintUnformatted(root);
    free(json_string);
    cJSON_Delete(root);

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
    static char json[1024];   // safe size for 5 measurements
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

        .auth_type = HTTP_AUTH_TYPE_NONE, // We use API key in headers, no HTTP auth
        .disable_auto_redirect = true,
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return false;
    }

    esp_http_client_set_header(client, "apikey", SUPABASE_API_KEY);
    // Note: For Supabase, the API key is enough for authentication.
    // esp_http_client_set_header(client, "Authorization",
    //                            "Bearer " SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    // Optional: ask Supabase to return the created records in the response for debugging
    esp_http_client_set_header(client, "Prefer", "return=representation");

    size_t json_len = strlen(json);
    // ESP_LOGW(TAG, "JSON length = %d", (int)json_len);
    // ESP_LOGW(TAG, "JSON bytes:");
    // ESP_LOG_BUFFER_CHAR(TAG, json, json_len);


    // esp_http_client_open(client, json_len); // No need to specify content length when using set_post_field
    
    // esp_http_client_write(client, json, json_len);

    // // ✅ Just read status, ignore auth machinery
    // int status = esp_http_client_get_status_code(client);

    // esp_http_client_close(client);
    // esp_http_client_cleanup(client);



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

    // Optional: read response body for debugging (Supabase may return useful error info in the body)
    static char resp_buf[512];
    int resp_len = esp_http_client_read_response(
        client,
        resp_buf,
        sizeof(resp_buf) - 1
    );

    if (resp_len > 0) {
        resp_buf[resp_len] = '\0';  // Null-terminate
        ESP_LOGW(TAG, "HTTP response body: %s", resp_buf);
    }
    else{
        ESP_LOGW(TAG, "No response body or failed to read");
    }

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