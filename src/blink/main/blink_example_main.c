/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/i2c_master.h"
#include <ssd1306.h>
#include "display.h"
#include "ens160_aht21.h"
#include "esp_system.h"
#include "bmp280.h"
#include "esp_timer.h"

// Include WiFi manager for network connectivity
#include "wifi_manager.h"
// Include time manager for NTP time handling
#include "time_manager.h"

// Include measurement storage and scheduler
#include "measurement.h"
#include "measurement_scheduler.h"

//TinyUSB includes for USB CDC test (USB Serial) 
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

// Include upload manager for handling data uploads to Supabase
#include "upload_manager.h"

uint16_t tvoc, eco2;

static const char *TAG = "example";

static ens160_aht21_handle_t ens160_aht21_handle;
ssd1306_handle_t ssd1306_handle;
bmp280_handle_t bmp280_handle;
static esp_timer_handle_t page_timer_handle;

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 15


// Variables to hold sensor data for storage
static int64_t timestamp;
float temperature, humidity;
float pressure, altitude;
bool page_update_needed = true; // Force initial update on startup


static void PageUpdateTimerCallback(void *arg)
{
    page_update_needed = true;
    ESP_LOGW(TAG, "Page update timer expired, flag set");
}

void page_update_acknowledge(void)
{
    page_update_needed = false;
    ESP_LOGW(TAG, "Update acknowledged, flag cleared");
}

static void PageUpdateTimerStart(uint32_t period_seconds)
{
    esp_timer_create_args_t timer_args = {
        .callback = &PageUpdateTimerCallback,
        .name = "PageUpdateTimer"
    };

    ESP_ERROR_CHECK(
        esp_timer_create(&timer_args, &page_timer_handle)
    );

    ESP_ERROR_CHECK(
        esp_timer_start_periodic(
            page_timer_handle,
            (uint64_t)period_seconds * 1000000ULL
        )
    );

    ESP_LOGI(TAG, "Page update timer started (%lu s)", (unsigned long)period_seconds);
}

void app_main(void)
{
    //CDC test
    printf("USB CDC test\n");

    // Initialize I2C bus for OLED (I2C_NUM_0 on pins 11/12)
    i2c_master_bus_config_t i2c_bus_config_oled = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = 12,
        .scl_io_num = 11,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
    };
    i2c_master_bus_handle_t i2c_bus_handle_oled;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config_oled, &i2c_bus_handle_oled));

    // Initialize SSD1306 display
    ssd1306_config_t ssd1306_config = I2C_SSD1306_128x64_CONFIG_DEFAULT;
    ESP_ERROR_CHECK(ssd1306_init(i2c_bus_handle_oled, &ssd1306_config, &ssd1306_handle));

    // Clear display and display a message
    ssd1306_clear_display(ssd1306_handle, false);
    ssd1306_display_text(ssd1306_handle, 0, "OLED Ready", false);

    // Initialize WiFi and wait for connection
    wifi_manager_init();
    // Block until WiFi is connected (optional, can run in offline mode if connection fails)
    wifi_manager_wait_connected();

    // Display network status
    if (wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "Network features enabled");
    } else {
        ESP_LOGW(TAG, "Running in offline mode");
    }

    /* Initialize time handling */
    time_manager_init();

    /* Sync time if needed */
    if (!time_manager_sync_if_needed()) {
        ESP_LOGW(TAG, "Time not available yet, running offline");
    } else {
        int64_t now;
        if (time_manager_get_timestamp(&now)) {
            ESP_LOGI(TAG, "Current UTC time: %lld", (long long)now);
        }
    }
    // Variable to track display update frequency
    int loop_count = 0;

    // Initialize measurement scheduler to trigger every 30 seconds
    measurement_scheduler_init(30);  // 30 seconds

    PageUpdateTimerStart(10); // Update display every 10 seconds

    measurement_clear_all(); // Clear any existing measurements on startup for clean testing

    while (1) {

        // Read ENS160 + AHT21 and BMP280-M sensor data and update display every 20 iterations (2 seconds)
        if (page_update_needed) {
            
            page_update_acknowledge();

            //uint16_t tvoc, eco2;
            // if (ens160_aht21_read_all_data(&ens160_aht21_handle, &temperature, &humidity, &tvoc, &eco2) == ESP_OK) {
            
            // Initialize I2C bus for ENS160 + AHT21 (I2C_NUM_1 on pins 8/9)
            // SDA - GPIO8, SCL - GPIO9
            i2c_master_bus_config_t i2c_bus_config_env = {
                .i2c_port = I2C_NUM_1,
                .sda_io_num = 8,
                .scl_io_num = 9,
                .clk_source = I2C_CLK_SRC_DEFAULT,
                .glitch_ignore_cnt = 7,
                .intr_priority = 0,
            };
            i2c_master_bus_handle_t i2c_bus_handle_env;
            ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config_env, &i2c_bus_handle_env));

            // Initialize ENS160 + AHT21 sensor module
            esp_err_t env_init_result = ens160_aht21_init(&ens160_aht21_handle, i2c_bus_handle_env);
            bool env_available = (env_init_result == ESP_OK);

            if (!env_available) {
                ESP_LOGE(TAG, "ENS160/AHT21 initialization failed: %s", esp_err_to_name(env_init_result));
                ssd1306_clear_display(ssd1306_handle, false);
                ssd1306_display_text(ssd1306_handle, 0, "ENS160/AHT21", false);
                ssd1306_display_text(ssd1306_handle, 1, "Init Failed", false);
                ssd1306_display_text(ssd1306_handle, 2, "Check wiring", false);
                ssd1306_display_text(ssd1306_handle, 3, "ADD=3V3, CS=3V3", false);
            }

            // if (!env_available) {
            //     ssd1306_clear_display(ssd1306_handle, false);
            //     ssd1306_display_text(ssd1306_handle, 0, "ENS160/AHT21", false);
            //     ssd1306_display_text(ssd1306_handle, 1, "Not Available", false);
            // }

            if (ens160_aht21_read_all_data(&ens160_aht21_handle, &temperature, &humidity) == ESP_OK) {
            //display_sensor_data_pages(temperature, humidity, tvoc, eco2);
            i2c_master_bus_rm_device(ens160_aht21_handle.aht21_dev_handle); // Remove AHT21 device from bus before reinitializing for BMP280
            //i2c_master_bus_reset(i2c_bus_handle_env); // Reset the bus to clear any residual state from ENS160/AHT21
            i2c_del_master_bus(i2c_bus_handle_env); // Clean up the bus handle before reinitializing for BMP280 
            gpio_reset_pin(8); // Reset SDA pin to clear any residual state from ENS160/AHT21
            gpio_reset_pin(9); // Reset SCL pin to clear any residual state from ENS160/AHT21
            // If BMP280 is available, read pressure and altitude and pass to display function
            // Initialize I2C bus for BMP280 (I2C_NUM_1 on pins 3/5) - REUSED from ENS160/AHT21 since they are on the same bus
            i2c_master_bus_config_t i2c_bus_config_bmp = {
                .i2c_port = I2C_NUM_1,
                .sda_io_num = 5,
                .scl_io_num = 3,
                .clk_source = I2C_CLK_SRC_DEFAULT,
                .glitch_ignore_cnt = 7,
                .intr_priority = 0,
            };
            i2c_master_bus_handle_t i2c_bus_handle_bmp;
            ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config_bmp, &i2c_bus_handle_bmp));
            
            vTaskDelay(pdMS_TO_TICKS(100)); // Short delay to ensure bus is ready before BMP280 initialization

            // Initialize BMP280
            //
            esp_err_t bmp_init = bmp280_init(&bmp280_handle, i2c_bus_handle_bmp);
            bool bmp_available = (bmp_init == ESP_OK);

            if (!bmp_available) {
                ESP_LOGE(TAG, "BMP280 initialization failed");
            }

            bmp280_read_compensated_data(&bmp280_handle, &temperature, &pressure, &altitude);

            display_sensor_data_pages(
                temperature,
                humidity,
                pressure,
                altitude
            );

            ESP_LOGI(TAG, "Temperature: %.2f C, Humidity: %.2f %%, Pressure: %.2f hPa, Altitude: %.2f m",
                        temperature, humidity, pressure, altitude);

            i2c_master_bus_rm_device(bmp280_handle.dev_handle); // Remove BMP280 device from bus after reading
            i2c_del_master_bus(i2c_bus_handle_bmp); // Clean up the bus handle after BMP280
            gpio_reset_pin(3); // Reset SDA pin after BMP280
            gpio_reset_pin(5); // Reset SCL pin after BMP280
            }

            time_manager_get_timestamp(&timestamp);

        }
        
        /* Measurement logic */
        if (measurement_scheduler_should_measure()) {

            ESP_LOGI(TAG, "Measurement triggered");

            measurement_scheduler_acknowledge();

            measurement_t m;

            m.timestamp_utc = timestamp;

            m.temperature_c_x100 = temperature * 100;
            m.humidity_x100      = humidity * 100;
            m.pressure_hpa_x100  = pressure * 100;
            m.altitude_m_x10     = altitude * 10;

            /* ENS160 inactive for now */
            m.tvoc_ppb = -1;
            m.eco2_ppm = -1;

            measurement_store(&m);

            if (measurement_count() >= 5) {
                // ready to upload
                ESP_LOGI(TAG, "Ready to upload measurements (have %d)", measurement_count());
                
                // For demonstration, read back the first 5 measurements and log them
                measurement_t batch[5];
                for (int i = 0; i < 5; i++) {
                    measurement_get(i, &batch[i]);
                    ESP_LOGI(TAG, "Measurement %d: Time=%lld, Temp=%.2f C, Humidity=%.2f %%, Pressure=%.2f hPa, Altitude=%.2f m",
                             i,
                             (long long)batch[i].timestamp_utc,
                             batch[i].temperature_c_x100 / 100.0,
                             batch[i].humidity_x100 / 100.0,
                             batch[i].pressure_hpa_x100 / 100.0,
                             batch[i].altitude_m_x10 / 10.0);
                }

                // Attempt to upload one batch of measurements (5 in this case)
                upload_manager_try_upload_one_batch();

                // After uploading, delete the uploaded measurements to free up space (FIFO)
                //measurement_delete(5);
            }
            else{
                ESP_LOGI(TAG, "Not enough measurements stored yet for upload (have %d, need 5)", measurement_count());
            }
        }

        loop_count++;
        vTaskDelay((CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS ) / 2); // Delay for half the blink period to achieve a full on-off cycle
    }
}
