#include "time_manager.h"

#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>

static const char *TAG = "time_mgr";

/* Sync once per day */
#define NTP_SYNC_INTERVAL_SEC (24 * 60 * 60)

/* Timeout waiting for SNTP */
#define NTP_SYNC_TIMEOUT_SEC 15

/* Last successful NTP sync (kept in RTC memory automatically) */
static int64_t last_ntp_sync = 0;

/* ---------------- INTERNAL HELPERS ---------------- */

static bool is_time_reasonable(time_t now)
{
    /* Epoch time corresponding to ~2024-01-01 */
    return (now > 1704067200);
}

static void initialise_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

    /* You can add backup servers if you want */
    esp_sntp_setservername(0, "pool.ntp.org");

    esp_sntp_init();
}

/* ---------------- PUBLIC API ---------------- */

void time_manager_init(void)
{
    /* Nothing to do yet; RTC time persists across deep sleep */
    ESP_LOGI(TAG, "Time manager initialized");

    // Set timezone to Central European Time (CET/CEST) for correct local time display
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
}

bool time_manager_is_time_valid(void)
{
    time_t now;
    time(&now);

    return is_time_reasonable(now);
}

int64_t time_manager_get_last_sync(void)
{
    return last_ntp_sync;
}

bool time_manager_get_timestamp(int64_t *timestamp)
{
    if (!timestamp) {
        return false;
    }

    time_t now;
    time(&now);

    if (!is_time_reasonable(now)) {
        return false;
    }

    *timestamp = (int64_t)now;
    return true;
}

bool time_manager_sync_if_needed(void)
{
    time_t now;
    time(&now);

    bool time_valid = is_time_reasonable(now);

    if (time_valid) {
        /* Already valid — check age */
        if ((now - last_ntp_sync) < NTP_SYNC_INTERVAL_SEC && last_ntp_sync != 0) {
            ESP_LOGI(TAG, "Time valid and recently synced, skipping NTP");
            return true;
        }
    }

    ESP_LOGI(TAG, "Starting NTP sync");

    initialise_sntp();

    time_t sync_start = esp_timer_get_time() / 1000000;
    while (!time_manager_is_time_valid()) {

        if ((esp_timer_get_time() / 1000000 - sync_start) > NTP_SYNC_TIMEOUT_SEC) {
            ESP_LOGE(TAG, "NTP sync timed out");
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    time(&now);
    last_ntp_sync = now;

    ESP_LOGI(TAG, "NTP sync successful, epoch=%lld", (long long)now);
    return true;
}