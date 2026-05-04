#pragma once
#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

#include <stdbool.h>

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
upload_result_t upload_manager_try_upload_one_batch(void);
    
upload_result_t upload_manager_get_last_result(void);


#endif // UPLOAD_MANAGER_H