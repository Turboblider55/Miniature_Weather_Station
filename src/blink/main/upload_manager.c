#include "upload_manager.h"

#include "measurement.h"
#include "wifi_manager.h"
#include "time_manager.h"

#define TAG "upload"

static upload_result_t last_upload_result = UPLOAD_SKIPPED;


upload_result_t upload_manager_get_last_result(void)
{
    return last_upload_result;
}

static char response_buffer[256];
static int response_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                int copy_len = evt->data_len;

                if (response_len + copy_len < sizeof(response_buffer)) {
                    memcpy(response_buffer + response_len,
                           evt->data,
                           copy_len);
                    response_len += copy_len;
                }
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}


static bool build_json_payload_for_batch(char *buf, size_t buf_len, uint32_t count)
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
        ESP_LOGE(TAG, "Failed to print JSON for batch");
        cJSON_Delete(root);
        return false;
    }

    snprintf(buf, buf_len, "%s", json_string);
    // buf = cJSON_PrintUnformatted(root);
    free(json_string);
    cJSON_Delete(root);

    return true;
}   

static bool build_json_payload_for_station_registration(char *buf, size_t buf_len, const char *station_name)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "name", station_name);

    char *json_string = cJSON_Print(obj);
    if (!json_string) {
        ESP_LOGE(TAG, "Failed to print JSON for station registration");
        cJSON_Delete(obj);
        return false;
    }

    snprintf(buf, buf_len, "%s", json_string);
    free(json_string);
    cJSON_Delete(obj);

    return true;
}

static bool build_json_payload_for_online_status_update(char *buf, size_t buf_len, bool online)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddBoolToObject(obj, "online", online);

    char *json_string = cJSON_Print(obj);
    if (!json_string) {
        ESP_LOGE(TAG, "Failed to print JSON for online status update");
        cJSON_Delete(obj);
        return false;
    }

    snprintf(buf, buf_len, "%s", json_string);
    free(json_string);
    cJSON_Delete(obj);

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
        if (!build_json_payload_for_batch(json, sizeof(json), BATCH_SIZE)) {
            ESP_LOGE(TAG, "Failed to build JSON payload");
            last_upload_result = UPLOAD_UNKNOWN_ERROR;
            return last_upload_result;
        }

        int json_len = strlen(json);
        
        ESP_LOGI(TAG, "Uploading batch: %s", json);

        char url[256];
        snprintf(url, sizeof(url),
                 "%s/measurements",
                 SUPABASE_URL);

        /* ---- HTTP CLIENT CONFIG ---- */
        esp_http_client_config_t config = {
            .url = url,
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

        if (response_len > 0) {
            response_buffer[response_len] = '\0';  // Null-terminate
            ESP_LOGW(TAG, "HTTP response body: %s", response_buffer);
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

esp_err_t upload_manager_register_station(void)
{
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot register station");
        return ESP_FAIL;
    }

    static char url[256];

    snprintf(url, sizeof(url), "%s/stations", SUPABASE_URL);

    ESP_LOGI(TAG, "Registering station with URL: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 8000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "apikey", SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Authorization","Bearer " SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Prefer", "return=representation");
    esp_http_client_set_header(client, "Range", "0-1");
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
    esp_http_client_set_header(client, "User-Agent", "ESP32-weather-station ID: "__STRINGIFY(STATION_ID));
    esp_http_client_set_header(client, "Connection", "close");
    esp_http_client_set_header(client, "Accept", "*/*");

    ESP_LOGI(TAG, "Registering station with name '%s'", STATION_NAME);

    static char json[256];
    if (!build_json_payload_for_station_registration(json, sizeof(json), STATION_NAME)) {
        ESP_LOGE(TAG, "Failed to build JSON payload for station registration");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Registering station with payload: %s", json);

    esp_http_client_set_post_field(client, json, strlen(json));

    //Before calling perform, reset response buffer and length to capture the response in the event handler
    response_len = 0;
    memset(response_buffer, 0, sizeof(response_buffer));

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Insert failed");
        esp_http_client_cleanup(client);
        return err;
    }
    else{
        ESP_LOGI(TAG, "Station registration HTTP status = %d", esp_http_client_get_status_code(client));
    }
    
    response_buffer[response_len] = '\0';

    ESP_LOGI(TAG, "Response: %s", response_buffer);

    int status = esp_http_client_get_status_code(client);

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if(status != 200 && status != 201 && status != 0 && response_len < 5){
        ESP_LOGE(TAG, "Failed to register station, HTTP status = %d", status);
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(response_buffer);
    if (root && cJSON_GetArraySize(root) > 0) {
        cJSON *item = cJSON_GetArrayItem(root, 0);
        cJSON *id = cJSON_GetObjectItem(item, "id");
        if (cJSON_IsNumber(id)) {
            STATION_ID = id->valueint;
        }
    }
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Registered station with ID %d", STATION_ID);

    return ESP_OK;

}

esp_err_t upload_manager_set_online_status(bool online)
{
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot register station");
        return ESP_FAIL;
    }

    if (STATION_ID < 0) {
        fetch_station_id_by_name(STATION_NAME, &STATION_ID);
        if (STATION_ID < 0) {
            upload_manager_register_station();
            if (STATION_ID < 0) {
                ESP_LOGE(TAG, "Failed to register station, cannot set online status");
                return ESP_FAIL;
            }
        }
    }

    ESP_LOGI(TAG,"Station ID: %d",STATION_ID);

    ESP_LOGI(TAG, "Setting station %d online status to %s", STATION_ID, online ? "TRUE" : "FALSE");

    static char url[256];
    snprintf(url, sizeof(url),
             "%s/stations?id=eq.%d",
             SUPABASE_URL,
             STATION_ID);

    ESP_LOGI(TAG, "Updating online status with URL: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PATCH,
        .timeout_ms = 8000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "apikey", SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    static char patch_json[128];
    
    build_json_payload_for_online_status_update(patch_json, sizeof(patch_json), online);

    ESP_LOGI(TAG, "Updating station online status with payload: %s", patch_json);

    esp_http_client_set_post_field(client, patch_json, strlen(patch_json));

    esp_err_t err = esp_http_client_perform(client);

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update station online status");
        return err;
    }

    ESP_LOGI(TAG, "Station %d online status set to %s", STATION_ID, online ? "TRUE" : "FALSE");

    return ESP_OK;
}

esp_err_t fetch_station_id_by_name(const char *name, int *station_id)
{
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, cannot fetch station ID");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Fetching station ID for station '%s'", name);

    static char url[256];
    snprintf(url, sizeof(url),
             "%s/stations?name=eq.%s",
             SUPABASE_URL,
             name);

    ESP_LOGI(TAG, "Fetching station ID with URL: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 8000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "apikey", SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization","Bearer " SUPABASE_API_KEY);
    esp_http_client_set_header(client, "Prefer", "return=representation");
    esp_http_client_set_header(client, "Range", "0-1");
    esp_http_client_set_header(client, "User-Agent", " Requesting station ID for station name: " __STRINGIFY(name));
    esp_http_client_set_header(client, "Connection", "close");
    esp_http_client_set_header(client, "Accept", "*/*");

    //Before calling perform, reset response buffer and length to capture the response in the event handler
    response_len = 0;
    memset(response_buffer, 0, sizeof(response_buffer));

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GET station failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int status = esp_http_client_get_status_code(client);

    ESP_LOGI(TAG, "HTTP status for station id fetch = %d", status);

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    *station_id = -1;

    ESP_LOGI(TAG, "Response for station ID fetch: %s", response_buffer);

    if ((status == 200  || status == 201 || status == 0) && response_len > 5) {
        /* Parse JSON to extract ID */
        cJSON *root = cJSON_Parse(response_buffer);

        ESP_LOGI(TAG, "printing json root: %s", cJSON_Print(root));

        if (root && cJSON_GetArraySize(root) > 0) {
            cJSON *item = cJSON_GetArrayItem(root, 0);
            cJSON *id = cJSON_GetObjectItem(item, "id");
            if (cJSON_IsNumber(id)) {
                *station_id = id->valueint;
            }
        }
        cJSON_Delete(root);
    }

    if (*station_id < 0) {
        ESP_LOGW(TAG, "Station '%s' not found", name);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Fetched station ID %d for station '%s'", *station_id, name);
    return ESP_OK;
}
