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

//TinyUSB includes for USB CDC test (USB Serial) 
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static const char *TAG = "example";

static ens160_aht21_handle_t ens160_aht21_handle;
ssd1306_handle_t ssd1306_handle;
bmp280_handle_t bmp280_handle;

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 15

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 255, 255, 255);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);

        //ssd1306_clear_display(ssd1306_handle, false);
        //ssd1306_display_text(ssd1306_handle, 0, "LED ON", false);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
        //ssd1306_clear_display(ssd1306_handle, false);
        //ssd1306_display_text(ssd1306_handle, 0, "LED OFF", false);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();

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


    // Variable to track display update frequency
    int loop_count = 0;

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;

        // Read ENS160 + AHT21 sensor data and update display every 20 iterations (2 seconds)
        if (loop_count % 20 == 0) {
                float temperature, humidity;
                float pressure, altitude;
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

                if (!env_available) {
                    ssd1306_clear_display(ssd1306_handle, false);
                    ssd1306_display_text(ssd1306_handle, 0, "ENS160/AHT21", false);
                    ssd1306_display_text(ssd1306_handle, 1, "Not Available", false);
                }

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

        //This drawing function takes a long time to execute, so it is placed after the LED state change and before the delay.
        //How could i optimize this? I want to draw the circles faster, but i don't want to change the LED state change and delay order.
        //One possible optimization is to reduce the number of circles being drawn. Instead of drawing 5 circles with different radii, you could draw only one circle with a larger radius. This would significantly reduce the execution time while still providing a visual effect on the OLED display.
        //Another optimization could be to use a more efficient algorithm for drawing circles, such as Bresenham's circle algorithm, which can reduce the number of calculations needed to draw a circle.
        // Additionally, you could consider using a buffer to draw the circles off-screen and then update the display in one go, which can also improve performance.
        // ssd1306_display_circle(ssd1306_handle, 64, 32, 20, false);
        // ssd1306_display_circle(ssd1306_handle, 64, 32, 16, false);
        // ssd1306_display_circle(ssd1306_handle, 64, 32, 12, false);
        // ssd1306_display_circle(ssd1306_handle, 64, 32, 8, false);
        // ssd1306_display_circle(ssd1306_handle, 64, 32, 4, false);
        // ssd1306_draw_buffer(ssd1306_handle);


        }
        
        loop_count++;
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
