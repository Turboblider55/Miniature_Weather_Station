#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

// Initialize WiFi in station mode and start connection
void wifi_manager_init(void);

// Returns true if connected to WiFi
bool wifi_manager_is_connected(void);

// Block until WiFi is connected (optional)
void wifi_manager_wait_connected(void);

// Returns true if WiFi connection failed after retries
bool wifi_manager_is_failed(void);

const char *wifi_manager_get_ssid(void);

int wifi_manager_get_retry_count(void);

int wifi_manager_get_max_retry(void);

bool wifi_manager_is_connected(void);

int wifi_manager_get_rssi(void);

#endif // WIFI_MANAGER_H