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

#include "device_state.h" // Include device state management (e.g., for handling different modes like MEASURE, UPLOAD, DISPLAY, IDLE)

#include "bh1750.h"
#include "cloudiness.h"

#include "esp_sleep.h"

// Include status LED handling
#include "status_led.h"

uint16_t tvoc, eco2;

static const char *TAG = "weather_station";

static bool display_force_update = false;
static bool upload_in_progress = false;

static bool upload_just_happened = false;
static int upload_status_display_cycles = 0;
#define UPLOAD_STATUS_DISPLAY_CYCLES 1

#define WAKE_INTERVAL_SECONDS 180
static bool MEASUREMENT_TIME_SET_PROPERLY = true;
#define DISPLAY_ACTIVE_TIME_MS 3000   // 3 seconds display before sleep
const bool ESP_SHOULD_SLEEP = false;

//Default value unknown
esp_sleep_wakeup_cause_t cause = ESP_SLEEP_WAKEUP_UNDEFINED;

static ens160_aht21_handle_t ens160_aht21_handle;
ssd1306_handle_t ssd1306_handle;
bmp280_handle_t bmp280_handle;
bh1750_handle_t bh1750_handle;
static esp_timer_handle_t page_timer_handle;

measurement_t m;

// Global variable to hold the latest measurement for display and upload

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/

// Variables to hold sensor data for storage
static int64_t timestamp;
float temperature, humidity = 0;
float pressure, altitude = 0;
float lux = 0;
float cloud_index = 0;
bool page_update_needed = true; // Force initial update on startup

static int UploadAttempt = 0;
static int BatchCount = 1;

//RTC variable
RTC_DATA_ATTR int boot_count = 0;

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

bool TryToSyncTime(void){

    /* Initialize time handling */
    time_manager_init();

    /* Sync time if needed */
    if (!time_manager_sync_if_needed()) {
        ESP_LOGW(TAG, "Time not available yet, running offline");
        return false;
    } else {
        int64_t now;
        if (time_manager_get_timestamp(&now)) {
            ESP_LOGI(TAG, "Current UTC time: %lld", (long long)now);
        }
    }

    return true;
}

bool TryToConnectToWifi(void){
    // Initialize WiFi and wait for connection
    wifi_manager_init();
    // Block until WiFi is connected (optional, can run in offline mode if connection fails)
    wifi_manager_wait_connected();

    // Display network status
    if (wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "Network features enabled");
        return true;
    } else {
        ESP_LOGW(TAG, "Running in offline mode");
    }
    return false;
}

static bool perform_measurement(measurement_t *m)
{
    // This function can be used to trigger a measurement outside of the regular scheduler, if needed
    // For now, we rely on the measurement scheduler to trigger measurements at regular intervals
    ESP_LOGI(TAG, "Measurement triggered");
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

    vTaskDelay(pdMS_TO_TICKS(20)); // Short delay to ensure bus is ready before sensor initialization

    // Initialize ENS160 + AHT21 sensor module
    esp_err_t env_init_result = ens160_aht21_init(&ens160_aht21_handle, i2c_bus_handle_env);

    if (env_init_result != ESP_OK) {
        ESP_LOGE(TAG, "ENS160/AHT21 initialization failed: %s", esp_err_to_name(env_init_result));
        ssd1306_clear_display(ssd1306_handle, false);
        ssd1306_display_text(ssd1306_handle, 0, "ENS160/AHT21", false);
        ssd1306_display_text(ssd1306_handle, 1, "Init Failed", false);
        ssd1306_display_text(ssd1306_handle, 2, "Check wiring", false);
        ssd1306_display_text(ssd1306_handle, 3, "ADD=3V3, CS=3V3", false);

        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); //Short delay so sensor has time to start up

    esp_err_t status = ens160_aht21_read_all_data(&ens160_aht21_handle, &temperature, &humidity);

    if(status != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from ENS160/AHT21: %s", esp_err_to_name(status));
        ssd1306_clear_display(ssd1306_handle, false);
        ssd1306_display_text(ssd1306_handle, 0, "ENS160/AHT21", false);
        ssd1306_display_text(ssd1306_handle, 1, "Read Failed", false);  
        return false;
    }

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
    
    vTaskDelay(pdMS_TO_TICKS(20)); // Short delay to ensure bus is ready before BMP280 initialization

    // Initialize BMP280
    //
    esp_err_t bmp_init = bmp280_init(&bmp280_handle, i2c_bus_handle_bmp);
    
    if (bmp_init != ESP_OK) {
        ESP_LOGE(TAG, "BMP280 initialization failed");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); //Short delay so sensor has time to start up

    bmp280_read_compensated_data(&bmp280_handle, &temperature, &pressure, &altitude);

    i2c_master_bus_rm_device(bmp280_handle.dev_handle); // Remove BMP280 device from bus after reading
    i2c_del_master_bus(i2c_bus_handle_bmp); // Clean up the bus handle after BMP280
    gpio_reset_pin(3); // Reset SDA pin after BMP280
    gpio_reset_pin(5); // Reset SCL pin after BMP280

    i2c_master_bus_handle_t i2c_bus_handle_light;
    i2c_master_bus_config_t i2c_bus_cfg_light = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = 4,
        .scl_io_num = 2,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg_light, &i2c_bus_handle_light));
    ESP_ERROR_CHECK(bh1750_init(&bh1750_handle, i2c_bus_handle_light));

    vTaskDelay(pdMS_TO_TICKS(20)); // Short delay to ensure bus is ready before BH1750 reading

    if (bh1750_read_lux(&bh1750_handle, &lux) == ESP_OK) {
        ESP_LOGI(TAG, "BH1750 light level: %.2f lux", lux);
    } else {
        ESP_LOGE(TAG, "Failed to read BH1750 light level");
        lux = -1; // Indicate error in reading lux
    }

    // if(lux > CLOUDINESS_REFERENCE_LUX){
    //     CLOUDINESS_REFERENCE_LUX = lux; // Update reference lux if current reading is higher (indicating clearer sky)
    // }

    i2c_master_bus_rm_device(bh1750_handle.dev_handle); // Remove BH1750 device from bus after reading
    i2c_del_master_bus(i2c_bus_handle_light); // Clean up the bus handle after BH1750
    gpio_reset_pin(2); // Reset SDA pin after BH1750
    gpio_reset_pin(4); // Reset SCL pin after BH1750
    
    time_manager_get_timestamp(&timestamp);

    //Convert lux to cloud index (0-100 scale) based on reference lux for clear sky at the current location and time
    cloud_index = 100 - calculate_cloud_index(lux, CLOUDINESS_REFERENCE_LUX) * 100;  // Invert cloud index so that higher values mean clearer skies

    // Update global measurement struct for display and upload
    m->timestamp_utc = timestamp;

    m->temperature_c_x100 = (uint16_t) (temperature * 100); //first multiply, then convert
    m->humidity_x100      = (uint16_t) (humidity * 100);
    m->pressure_hpa_x100  = (uint32_t) (pressure * 100);
    m->altitude_m_x10     = (uint32_t) (altitude * 10);

    /* ENS160 inactive for now */
    m->tvoc_ppb = -1;
    m->eco2_ppm = -1;

    //Lux and cloud index are already converted to fixed-point in perform_measurement()
    // m.lux_x100 *= 100; // Convert to fixed-point representation for storage
    // m.cloud_index *= 100; // Convert to fixed-point representation for storage

    m->lux = (int32_t)(lux); // Convert to fixed-point representation for storage
    m->cloud_index = (uint16_t)(cloud_index); // Convert to fixed-point representation for storage

    ESP_LOGI(TAG,"Time=%lld, Temp=%.2f °C, Humidity=%.2f %%, Pressure=%.2f hPa, Altitude=%.2f m, Lux=%.2f, Cloud Index=%.2f",
                m->timestamp_utc,
                m->temperature_c_x100 / 100.0f,
                m->humidity_x100 / 100.0f,
                m->pressure_hpa_x100 / 100.0f,
                m->altitude_m_x10 / 10.0f,
                (float)m->lux,
                (float)m->cloud_index 
            );

    return true;
}

static bool latest_measurement_valid = false;

void handle_state_measure_and_store(void)
{
    if (measurement_scheduler_should_measure() || (ESP_SHOULD_SLEEP && cause != ESP_SLEEP_WAKEUP_UNDEFINED)) {

        ESP_LOGI(TAG, "Measurement triggered");

        measurement_scheduler_acknowledge();

        status_led_set(LED_STATE_MEASURE);

        if(!MEASUREMENT_TIME_SET_PROPERLY){
            //Measurement time was not set properly, stop current timer -> delete current timer -> set new timer with right interval
            ESP_ERROR_CHECK(delete_timer());

            measurement_scheduler_init(WAKE_INTERVAL_SECONDS);  // WAKE_INTERNAL_SECONDS seconds - 3 minutes
            MEASUREMENT_TIME_SET_PROPERLY = true;
        }

        perform_measurement(&m); // Read sensors and update global variables

        latest_measurement_valid = true;

        measurement_store(&m);

        if (measurement_count() < 5 * BatchCount) {
            // Not enough measurements to upload yet, go back to measuring
            ESP_LOGI(TAG, "Not enough measurements stored yet for upload (have %d, need %d)", measurement_count(), BATCH_SIZE * BatchCount);
            device_state = STATE_DISPLAY;
            return;
        }
    }

    device_state = STATE_UPLOAD;
}
static bool upload_attempted_this_cycle = false;

void handle_state_upload(void)
{
    if(measurement_count() >= 5 * BatchCount) {

        if(!wifi_manager_is_connected()){

            bool TryResult = TryToConnectToWifi();

            //If there's no wifi connection, dont even try to upload, increase needed batch size and skip upload
            if(!TryResult){
                ESP_LOGW(TAG, "Skipping upload, WiFi not connected");
                BatchCount++;
                device_state = STATE_DISPLAY;

                status_led_set(LED_STATE_ERROR);

                return;
            }
        }

        //TryToSyncTime();
        status_led_set(LED_STATE_UPLOAD);

        // ready to upload
        ESP_LOGI(TAG, "Ready to upload measurements (have %d)", measurement_count());
        
        // For demonstration, read back the first 5 measurements and log them
        measurement_t batch[5];
        for (int i = 0; i < 5; i++) {
            measurement_get(i, &batch[i]);
            ESP_LOGI(TAG, "Measurement %d: Time=%lld, Temp=%.2f C, Humidity=%.2f %%, Pressure=%.2f hPa, Altitude=%.2f m, Lux=%.2f, Cloud Index=%.2f",
                i,
                (long long)batch[i].timestamp_utc,
                batch[i].temperature_c_x100 / 100.0f,
                batch[i].humidity_x100 / 100.0f,
                batch[i].pressure_hpa_x100 / 100.0f,
                batch[i].altitude_m_x10 / 10.0f,
                (float)batch[i].lux ,
                (float)batch[i].cloud_index 
            );
        }

        // Attempt to upload one batch of measurements (5 in this case)

        upload_in_progress = true;
        display_force_update = true;

        upload_result_t res = upload_manager_try_upload_one_batch(&BatchCount);

        //Upload failed, increase attempt count
        if(res == UPLOAD_SKIPPED || res == UPLOAD_AUTH_ERROR || res == UPLOAD_NET_ERROR || res == UPLOAD_SERVER_ERROR){
            UploadAttempt++;
        }
        else if(res == UPLOAD_OK){
            UploadAttempt = 0; //Reset attempt count after successful upload
            status_led_set(LED_STATE_SUCCESS); // Indicate success with LED pattern
        }
        //Tried to upload 5 or more times, increase needed batch count so it wont get stuck at uploading
        if(UploadAttempt >= 5){
            BatchCount++;
            UploadAttempt = 0;

            status_led_set(LED_STATE_ERROR); // Indicate error with LED pattern

        }

        upload_in_progress = false;
        display_force_update = true;

        // After uploading, set a flag to show upload status on the display for a few cycles
        upload_just_happened = true;
        upload_status_display_cycles = UPLOAD_STATUS_DISPLAY_CYCLES;
    }
    device_state = STATE_DISPLAY;
}

void handle_state_display(void)
{
    if (page_update_needed || display_force_update) {
            
        display_force_update = false;
        page_update_acknowledge();

        if(upload_in_progress) {
            display_upload_status_page();
            return; // Skip normal sensor display updates while upload is in progress
        }

        // Displaying upload status page if an upload just happened, for a few cycles
        if (upload_just_happened && upload_status_display_cycles > 0) {
            display_upload_status_page();
            upload_status_display_cycles--;

            if (upload_status_display_cycles == 0) {
                upload_just_happened = false;
            }

            // Stop here, do NOT render normal sensor pages this cycle
            vTaskDelay(pdMS_TO_TICKS(10));
            return;
        }

        display_sensor_data_pages(
            m.temperature_c_x100 / 100.f,
            m.humidity_x100 / 100.f,
            m.pressure_hpa_x100 / 100.f,
            m.altitude_m_x10 / 10.f,
            m.lux
        );

        ESP_LOGI(TAG, "Temperature: %.2f C, Humidity: %.2f %%, Pressure: %.2f hPa, Altitude: %.2f m, Lux: %.2f lux, Cloud Index: %.2f",
                    (float)m.temperature_c_x100 / 100.0f,
                    (float)m.humidity_x100  / 100.0f,
                    (float)m.pressure_hpa_x100  / 100.0f,
                    (float)m.altitude_m_x10  / 10.0f,
                    (float)m.lux ,
                    (float)m.cloud_index 
                );
        
    }
     
    device_state = STATE_IDLE;
}

void handle_state_idle(void)
{
    upload_attempted_this_cycle = false;
    latest_measurement_valid = false;

    if(ESP_SHOULD_SLEEP){

        ESP_LOGI(TAG, "Entering pre-sleep display phase");

        // Configure timer wakeup

        int64_t now;
        time_manager_get_timestamp(&now);

        int64_t next = ((now / WAKE_INTERVAL_SECONDS) + 1) * WAKE_INTERVAL_SECONDS;
        int64_t sleep_time = next - now;

        //If sleep time is more than 5 seconds it it worth going to sleep, otherwise not 
        if (sleep_time > 5) {
            
            //sleep_time = WAKE_INTERVAL_SECONDS;
            
            ESP_LOGI(TAG, "Preparing deep sleep for %d seconds", sleep_time);

            esp_sleep_enable_timer_wakeup(sleep_time * 1000000ULL);

            // Show final display for a short time
            vTaskDelay(pdMS_TO_TICKS(DISPLAY_ACTIVE_TIME_MS));

            led_off(); // Ensure LED is off before sleeping

            ESP_LOGI(TAG, "Entering deep sleep now...");
            vTaskDelay(pdMS_TO_TICKS(100)); // UART flush safety

            esp_deep_sleep_start();
        }
    }
    else{
        status_led_set(LED_STATE_IDLE);

        device_state = STATE_MEASURE;
    }
}

void app_main(void)
{
    //CDC test
    printf("USB CDC test\n");

    status_led_init();

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

    cause = esp_sleep_get_wakeup_cause();

    //If the wakeup cause is undefined, we have to try to connect to wifi because it is a fresh start, we have to know the time whether the device should sleep or not so we have to connect to wifi.
    //But if the device wakeup is known and we should sleep, there's no need to try to connect to the wifi because all every fresh start we sync time so there's no need to connect to the wifi every restart
    if(cause == ESP_SLEEP_WAKEUP_UNDEFINED || !ESP_SHOULD_SLEEP){
        if(TryToConnectToWifi() && TryToSyncTime()){
            ESP_LOGW(TAG, "Time sync successful, WiFi is connected. Changing online status to true.");
            upload_manager_set_online_status(true);
        }

    }

    if(!ESP_SHOULD_SLEEP && wifi_manager_is_connected()){
        //If the esp should not sleep we need to set up the timer properly, so we check the current time and set a time till the next measurement time
        int64_t now;
        time_manager_get_timestamp(&now);

        int64_t next = ((now / WAKE_INTERVAL_SECONDS) + 1) * WAKE_INTERVAL_SECONDS;
        int64_t sleep_time = next - now;

        measurement_scheduler_init(sleep_time);  // sleep_time seconds - 0 - 3 minutes
        MEASUREMENT_TIME_SET_PROPERLY = false;
    }

    // Initialize measurement scheduler to trigger every 180 seconds
    //This part is still questionable because of the deep sleep, if deep sleep is not needed when charging then this code is needed.
    
    PageUpdateTimerStart(30); // Update display every 30 seconds

    //Undefined wakeup cause -> Reset / restart after power loss or power down
    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGW(TAG, "Cold boot detected → clearing measurements");
        measurement_clear_all();
    } else {
        ESP_LOGI(TAG, "Wake from deep sleep → preserving measurements");
    }

    //Just for showing data and not waiting for the measurement scheduler to make it's first measurement
    perform_measurement(&m);

    //Proper bootup counting
    boot_count++;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    while (1) {
   
        switch (device_state) {

            case STATE_MEASURE:
                handle_state_measure_and_store();
                break;

            case STATE_UPLOAD:
                handle_state_upload();
                break;

            case STATE_DISPLAY:
                handle_state_display();
                break;

            case STATE_IDLE:
                handle_state_idle();
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
    }
}
