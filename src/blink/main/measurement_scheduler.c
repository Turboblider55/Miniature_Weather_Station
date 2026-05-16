#include "measurement_scheduler.h"
#include "esp_log.h"

static const char *TAG = "measure_sched";

/* Measurement flag */
static volatile bool measure_now = false;

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
        .name = MEASUREMENT_SCHEDULER_NAME
    };

    ESP_ERROR_CHECK(
        esp_timer_create(&timer_args, &measurement_timer_handle)
    );

    ESP_ERROR_CHECK(
        esp_timer_start_periodic(
            measurement_timer_handle,
            (uint64_t)period_seconds * 1000000ULL
        )
    );

    ESP_LOGI(TAG, "Measurement scheduler started (%lu s)",
             (unsigned long)period_seconds);
}

esp_err_t delete_timer(void){

    if(measurement_timer_handle != NULL){
        ESP_ERROR_CHECK(esp_timer_stop(measurement_timer_handle));
        ESP_ERROR_CHECK(esp_timer_delete(measurement_timer_handle));
        return ESP_OK;
    }
    else{
        ESP_LOGE(TAG,"measurement_timer_handle is NULL, stop and delete failed.");
    }

    return ESP_FAIL;
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