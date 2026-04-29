#include "measurement_scheduler.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "measure_sched";

/* Measurement flag */
static volatile bool measure_now = false;
static esp_timer_handle_t timer_handle;

/* Timer callback (IRAM-safe, minimal) */
static void measurement_timer_cb(void *arg)
{
    measure_now = true;
    ESP_LOGW(TAG, "Measurement timer expired, flag set");
}

void measurement_scheduler_init(uint32_t period_seconds)
{
    esp_timer_create_args_t timer_args = {
        .callback = &measurement_timer_cb,
        .name = "measurement_timer"
    };

    ESP_ERROR_CHECK(
        esp_timer_create(&timer_args, &timer_handle)
    );

    ESP_ERROR_CHECK(
        esp_timer_start_periodic(
            timer_handle,
            (uint64_t)period_seconds * 1000000ULL
        )
    );

    ESP_LOGI(TAG, "Measurement scheduler started (%lu s)",
             (unsigned long)period_seconds);
}

bool measurement_scheduler_should_measure(void)
{
    return measure_now;
}

void measurement_scheduler_acknowledge(void)
{
    measure_now = false;
    ESP_LOGW(TAG, "Measurement acknowledged, flag cleared");
}