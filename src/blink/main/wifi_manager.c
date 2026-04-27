#include "wifi_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define WIFI_SSID "Cudy-A64C"
#define WIFI_PASS "99957902"
#define WIFI_MAX_RETRY 5

static const char *TAG = "wifi";

static EventGroupHandle_t wifi_event_group;

static int s_retry_num = 0;
static int s_rssi = -100;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT    BIT1


static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        s_retry_num = 0;
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi started, connecting...");

    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {

        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retrying WiFi connection (%d/%d)",
                     s_retry_num, WIFI_MAX_RETRY);
        } else {
            ESP_LOGE(TAG, "WiFi connection failed after %d retries",
                     WIFI_MAX_RETRY);
            xEventGroupSetBits(wifi_event_group, WIFI_FAILED_BIT);
        }

        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

    } else if (event_base == IP_EVENT &&
            event_id == IP_EVENT_STA_GOT_IP) {

        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            s_rssi = ap.rssi;
        }

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }

}

void wifi_manager_init(void)
{
    esp_err_t ret;

    // NVS is required for WiFi
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init finished");
}

bool wifi_manager_is_connected(void)
{
    EventBits_t bits =
        xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT);
}

void wifi_manager_wait_connected(void)
{
    ESP_LOGI(TAG, "Waiting for WiFi connection...");

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
    } else if (bits & WIFI_FAILED_BIT) {
        ESP_LOGE(TAG, "WiFi failed, continuing without network");
    }
}

bool wifi_manager_is_failed(void)
{
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_FAILED_BIT);
}


const char *wifi_manager_get_ssid(void)
{
    return WIFI_SSID;
}

int wifi_manager_get_retry_count(void)
{
    return s_retry_num;
}

int wifi_manager_get_max_retry(void)
{
    return WIFI_MAX_RETRY;
}

int wifi_manager_get_rssi(void)
{
    return s_rssi;
}

