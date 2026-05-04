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
#include <esp_tls.h>

#define TAG "upload"

/* ---- CONFIG ---- */
#define BATCH_SIZE 5

#define SUPABASE_URL "https://hzucoiipjnfhnqjxtrgj.supabase.co/rest/v1/measurements"
#define SUPABASE_API_KEY "sb_publishable_47ApWRf7T1esYfIBUWkRGg_VVbAVhp3"

#define SUPABASE_HOST "hzucoiipjnfhnqjxtrgj.supabase.co"
#define SUPABASE_PORT 443
#define SUPABASE_PATH "/rest/v1/measurements"


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

    int json_len = strlen(json);
    
    ESP_LOGI(TAG, "Uploading batch: %s", json);

    static char request[2048];
    int req_len = snprintf(
        request,
        sizeof(request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: ESP32-weather-station ID: %d\r\n"
        "Accept: */*\r\n"
        "Accept-Encoding: identity\r\n"
        "Content-Type: application/json\r\n"
        "apikey: %s\r\n"
        //"Prefer: return=representation\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        SUPABASE_PATH,
        SUPABASE_HOST,
        STATION_ID,
        SUPABASE_API_KEY,
        json_len,
        json
    );

    if (req_len <= 0 || req_len >= sizeof(request)) {
        ESP_LOGE(TAG, "Request build failed");
        return false;
    }

    ESP_LOGI(TAG, "Connecting via TLS…");

    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    struct esp_tls *tls = esp_tls_init();
    if (!tls) {
        ESP_LOGE(TAG, "esp_tls_init failed");
        return false;
    }

    if (esp_tls_conn_new_sync(
            SUPABASE_HOST,
            strlen(SUPABASE_HOST),
            SUPABASE_PORT,
            &cfg,
            tls) != 1) {

        ESP_LOGE(TAG, "TLS connection failed");
        esp_tls_conn_destroy(tls);
        return false;
    }

    ESP_LOGI(TAG, "TLS connected, sending request");

    int written = esp_tls_conn_write(tls, request, req_len);
    if (written <= 0) {
        ESP_LOGE(TAG, "Write failed");
        esp_tls_conn_destroy(tls);
        return false;
    }

    char response[512];
    int read = esp_tls_conn_read(tls, response, sizeof(response) - 1);
    if (read > 0) {
        response[read] = '\0';
        ESP_LOGI(TAG, "Response:\n%.*s", read, response);
    }

    esp_tls_conn_destroy(tls);

    // Success if HTTP 201 or 200 appears
    if (strstr(response, "HTTP/1.1 201") || strstr(response, "HTTP/1.1 200")) {
        ESP_LOGI(TAG, "Upload successful");
        return true;
    }

    ESP_LOGE(TAG, "Upload failed");
    return false;


        // /* ---- HTTP CLIENT CONFIG ---- */
        // esp_http_client_config_t config = {
        //     .url = SUPABASE_URL,
        //     .method = HTTP_METHOD_POST,
        //     .timeout_ms = 10000,
        //     .transport_type = HTTP_TRANSPORT_OVER_SSL,
        //     .crt_bundle_attach = esp_crt_bundle_attach, // Use built-in CA bundle for server verification
        // };

        // esp_http_client_handle_t client =
        //     esp_http_client_init(&config);

        // if (!client) {
        //     ESP_LOGE(TAG, "Failed to init HTTP client");
        //     return false;
        // }

        // esp_http_client_set_header(client, "apikey", SUPABASE_API_KEY);
        // // Note: For Supabase, the API key is enough for authentication.
        // esp_http_client_set_header(client, "Authorization",
        //                            "Bearer " SUPABASE_API_KEY);
        // esp_http_client_set_header(client, "Content-Type", "application/json");
        // esp_http_client_set_header(client, "Accept-Encoding", "identity");
        // esp_http_client_set_header(client, "User-Agent", "ESP32-weather-station ID: "__STRINGIFY(STATION_ID));
        // esp_http_client_set_header(client, "Connection", "close");
        // esp_http_client_set_header(client, "Accept", "*/*");
        // esp_http_client_set_header(client, "Content-Length", NULL);  // esp_http_client will set this automatically when body is sent
        // // Optional: ask Supabase to return the created records in the response for debugging
        // // esp_http_client_set_header(client, "Prefer", "return=representation");

        // //esp_http_client_set_post_field(client, json, json_len);

        // // /* ---- SEND (BLOCKING, SAFE) ---- */
        // // esp_err_t err = esp_http_client_perform(client);

        // // if (err != ESP_OK) {
        // //     ESP_LOGE(TAG, "HTTP error: %s", esp_err_to_name(err));
        // //     esp_http_client_cleanup(client);
        // //     return false;
        // // }
        
        // /* Open connection with explicit length */
        // ESP_ERROR_CHECK(esp_http_client_open(client, json_len));

        // /* Write body explicitly */
        // int written = esp_http_client_write(client, json, json_len);
        // if (written != json_len) {
        //     ESP_LOGE(TAG, "HTTP write failed (%d/%d)", written, json_len);
        // }

        // /* Now the request is fully sent */

        // int status = esp_http_client_get_status_code(client);
        // ESP_LOGI(TAG, "HTTP status = %d", status);

        // // Optional: read response body for debugging (Supabase may return useful error info in the body)
        // static char resp_buf[512];
        // int resp_len = esp_http_client_read_response(
        //     client,
        //     resp_buf,
        //     sizeof(resp_buf) - 1
        // );

        // if (resp_len > 0) {
        //     resp_buf[resp_len] = '\0';  // Null-terminate
        //     ESP_LOGW(TAG, "HTTP response body: %s", resp_buf);
        // }
        // else{
        //     ESP_LOGW(TAG, "No response body or failed to read");
        // }

        // esp_http_client_close(client);
        // esp_http_client_cleanup(client);

        // /* ---- CONFIRM SUCCESS ---- */
        // if (status == 200 || status == 201) {
        //     ESP_LOGI(TAG, "Upload successful, deleting batch");
        //     measurement_delete(BATCH_SIZE);
        //     return true;
        // }

        // ESP_LOGW(TAG, "Upload failed, keeping data");
        // return false;
    
}