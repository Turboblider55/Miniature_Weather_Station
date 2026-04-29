#include "measurement.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "measurement";

/* ---------- Internal helpers ---------- */

static bool open_nvs(nvs_handle_t *handle)
{
    esp_err_t err = nvs_open(MEASUREMENT_NAMESPACE,
                             NVS_READWRITE,
                             handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS (%s)", esp_err_to_name(err));
        return false;
    }
    return true;
}

static uint32_t get_count(nvs_handle_t handle)
{
    uint32_t count = 0;
    nvs_get_u32(handle, "count", &count);
    return count;
}

static bool set_count(nvs_handle_t handle, uint32_t count)
{
    return nvs_set_u32(handle, "count", count) == ESP_OK;
}

/* ---------- Public API ---------- */

bool measurement_store(const measurement_t *m)
{
    if (!m) return false;

    nvs_handle_t handle;
    if (!open_nvs(&handle)) return false;

    uint32_t count = get_count(handle);

    if (count >= MEASUREMENT_MAX_COUNT) {
        ESP_LOGW(TAG, "Storage full, measurement dropped");
        nvs_close(handle);
        return false;
    }

    char key[16];
    snprintf(key, sizeof(key), "m_%lu", (unsigned long)count);

    esp_err_t err = nvs_set_blob(handle, key, m, sizeof(measurement_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store measurement (%s)",
                 esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    set_count(handle, count + 1);
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Measurement stored, total=%lu",
             (unsigned long)(count + 1));

    return true;
}

int measurement_count(void)
{
    nvs_handle_t handle;
    if (!open_nvs(&handle)) return 0;

    uint32_t count = get_count(handle);
    nvs_close(handle);

    return (int)count;
}

bool measurement_get(uint32_t index, measurement_t *out)
{
    if (!out) return false;

    nvs_handle_t handle;
    if (!open_nvs(&handle)) return false;

    uint32_t count = get_count(handle);
    if (index >= count) {
        nvs_close(handle);
        return false;
    }

    char key[16];
    snprintf(key, sizeof(key), "m_%lu", (unsigned long)index);

    size_t size = sizeof(measurement_t);
    esp_err_t err = nvs_get_blob(handle, key, out, &size);

    nvs_close(handle);
    return err == ESP_OK;
}

/*
 * Deletes the oldest `count` measurements (FIFO behavior)
 */
bool measurement_delete(uint32_t count)
{
    nvs_handle_t handle;
    if (!open_nvs(&handle)) return false;

    uint32_t total = get_count(handle);
    if (count > total) count = total;

    // Shift remaining measurements down
    for (uint32_t i = count; i < total; i++) {
        measurement_t temp;
        char from[16], to[16];

        snprintf(from, sizeof(from), "m_%lu", (unsigned long)i);
        snprintf(to,   sizeof(to),   "m_%lu", (unsigned long)(i - count));

        size_t size = sizeof(temp);
        if (nvs_get_blob(handle, from, &temp, &size) == ESP_OK) {
            nvs_set_blob(handle, to, &temp, sizeof(temp));
        }
    }

    // Remove old keys at the end
    for (uint32_t i = total - count; i < total; i++) {
        char key[16];
        snprintf(key, sizeof(key), "m_%lu", (unsigned long)i);
        nvs_erase_key(handle, key);
    }

    set_count(handle, total - count);
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Deleted %lu measurements", (unsigned long)count);
    return true;
}

void measurement_clear_all(void)
{
    nvs_handle_t handle;
    if (!open_nvs(&handle)) return;

    nvs_erase_all(handle);
    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGW(TAG, "All measurements cleared");
}