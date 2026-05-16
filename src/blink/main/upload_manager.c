#include "upload_manager.h"

#include "measurement.h"
#include "wifi_manager.h"
#include "time_manager.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

// For JSON construction (optional, can build manually if preferred)
#include "cJSON.h"
#include <esp_tls.h>

#define TAG "upload"

static upload_result_t last_upload_result = UPLOAD_SKIPPED;


upload_result_t upload_manager_get_last_result(void)
{
    return last_upload_result;
}


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
        cJSON_AddNumberToObject(obj, "lux", (int32_t)m.lux);
        cJSON_AddNumberToObject(obj, "cloud_index", (int16_t)m.cloud_index);
        cJSON_AddNumberToObject(obj,"eco2_ppm",m.eco2_ppm);
        cJSON_AddNumberToObject(obj,"tvoc_ppb",m.tvoc_ppb);
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
 
upload_result_t upload_manager_try_upload_one_batch(int *Batchcount)
{
    /* ---- GATING ---- */
    if (!wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "WiFi not connected, skipping upload");
        last_upload_result = UPLOAD_SKIPPED;
        return last_upload_result;
    }

    if (!time_manager_is_time_valid()) {
        ESP_LOGI(TAG, "Time not valid, skipping upload");
        last_upload_result = UPLOAD_SKIPPED;
        return last_upload_result;
    }

    int available = measurement_count();
    if (available < BATCH_SIZE) {
        ESP_LOGI(TAG, "Not enough measurements (%d/%d)",
                 available, BATCH_SIZE);
        last_upload_result = UPLOAD_SKIPPED;
        return last_upload_result;
    }
    for(size_t i = 0; i < fmin(MAX_BATCH_UPLOAD,*Batchcount); i++){
    
        /* ---- BUILD PAYLOAD ---- */
        static char json[1536];   // safe size for 5 measurements
        if (!build_json_payload(json, sizeof(json), BATCH_SIZE)) {
            ESP_LOGE(TAG, "Failed to build JSON payload");
            last_upload_result = UPLOAD_UNKNOWN_ERROR;
            return last_upload_result;
        }

        int json_len = strlen(json);
        
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
            last_upload_result = UPLOAD_UNKNOWN_ERROR;
            return last_upload_result;
        }

        esp_http_client_set_header(client, "apikey", SUPABASE_API_KEY);
        // Note: For Supabase, the API key is enough for authentication.
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "Accept-Encoding", "identity");
        esp_http_client_set_header(client, "User-Agent", "ESP32-weather-station ID: "__STRINGIFY(STATION_ID));
        esp_http_client_set_header(client, "Connection", "close");
        esp_http_client_set_header(client, "Accept", "*/*");
        //If prefer is set and supabase table has a RLS policy that does not allow returning the created record, the request will fail with 400 Bad Request, so use with caution and make sure your RLS policies allow it if you enable it.
        // esp_http_client_set_header(client, "Prefer", "return=representation");

        esp_http_client_set_post_field(client, json, json_len);

        /* ---- SEND (BLOCKING, SAFE) ---- */
        esp_err_t err = esp_http_client_perform(client);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP error: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            last_upload_result = UPLOAD_NET_ERROR;
            return last_upload_result;
        }

        /* Now the request is fully sent */

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

        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        
        /* ---- CONFIRM SUCCESS ---- */
        if (status == 200 || status == 201 || status == 0) {  // Some servers return 0 on success when using esp_http_client
            ESP_LOGI(TAG, "Upload successful, deleting batch");
            measurement_delete(BATCH_SIZE);
            last_upload_result = UPLOAD_OK;
            //return last_upload_result;
        }
        
        else if (status == 401 || status == 403) {
            ESP_LOGE(TAG,"Authentication error!");
            last_upload_result = UPLOAD_AUTH_ERROR;
            return last_upload_result;
        }

        else if (status >= 500) {
            ESP_LOGE(TAG,"Error on the server side!");
            last_upload_result = UPLOAD_SERVER_ERROR;
            return last_upload_result;
        }
        else { 
            ESP_LOGW(TAG, "Upload failed, keeping data");
            last_upload_result = UPLOAD_UNKNOWN_ERROR;
            return last_upload_result;
        }
        *Batchcount -= 1;
    }

  
    return last_upload_result;
}