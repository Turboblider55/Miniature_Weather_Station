#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize time manager (does not sync yet)
 */
void time_manager_init(void);

/**
 * @brief Trigger NTP sync if needed (WiFi must be connected)
 *
 * This will sync:
 *  - if time is invalid
 *  - if last sync was more than 24 hours ago
 *
 * @return true if time is valid after this call
 */
bool time_manager_sync_if_needed(void);

/**
 * @brief Check if system time is valid
 */
bool time_manager_is_time_valid(void);

/**
 * @brief Get current UTC time as epoch seconds
 *
 * @param[out] timestamp Pointer to receive epoch seconds
 * @return true if time is valid
 */
bool time_manager_get_timestamp(int64_t *timestamp);

/**
 * @brief Get last successful NTP sync time (epoch seconds)
 */
int64_t time_manager_get_last_sync(void);

#endif // TIME_MANAGER_H