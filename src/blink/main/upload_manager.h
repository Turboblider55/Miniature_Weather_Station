#pragma once
#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

#include <stdbool.h>
#include <esp_err.h>

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

// For JSON construction (optional, can build manually if preferred)
#include "cJSON.h"
#include <esp_tls.h>

/*
 * @brief Enum representing the result of an upload attempt.
 */
typedef enum {
    UPLOAD_OK = 0,
    UPLOAD_SKIPPED,
    UPLOAD_NET_ERROR,
    UPLOAD_AUTH_ERROR,
    UPLOAD_SERVER_ERROR,
    UPLOAD_UNKNOWN_ERROR
} upload_result_t;

/* ---- CONFIG ---- */
#define BATCH_SIZE 5

#define SUPABASE_URL CONFIG_SUPABASE_URL
#define STATION_NAME CONFIG_STATION_NAME
#define SUPABASE_API_KEY CONFIG_SUPABASE_API_KEY

static int STATION_ID = -1;   // Must exist in your stations table
#define MAX_BATCH_UPLOAD 4

/**
 * @brief Try to upload exactly one batch of measurements.
 *
 * Conditions checked internally:
 *  - WiFi must be connected
 *  - Time must be valid
 *  - At least BATCH_SIZE measurements stored
 *
 * @return UPLOAD_OK  Batch uploaded successfully and deleted
 * @return UPLOAD_SKIPPED Upload skipped (e.g., due to connectivity issues)
 * @return UPLOAD_NET_ERROR Network error occurred
 * @return UPLOAD_AUTH_ERROR Authentication error occurred
 * @return UPLOAD_SERVER_ERROR Server error occurred
 * @return UPLOAD_UNKNOWN_ERROR An unknown error occurred
 */
upload_result_t upload_manager_try_upload_one_batch(int *BatchCount);
    
upload_result_t upload_manager_get_last_result(void);

esp_err_t upload_manager_register_station(void);

esp_err_t upload_manager_set_online_status(bool online);

esp_err_t fetch_station_id_by_name(const char *name, int *station_id);


#endif // UPLOAD_MANAGER_H