#include "status_led.h"

static const char *LED = "status_led";
static led_strip_handle_t led_strip;

void led_on()  { led_strip_set_pixel(led_strip, 0, 255, 255, 255); led_strip_refresh(led_strip); }
void led_off() { led_strip_clear(led_strip); }

void status_led_init(void)
{
    gpio_reset_pin(LED_GPIO);

    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
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

void status_led_set(led_state_t state)
{
    switch(state)
    {
        case LED_STATE_BOOT:
            ESP_LOGI("LED", "LED_STATE_BOOT");
            for (int i=0;i<3;i++) {
                led_on(); vTaskDelay(100);
                led_off(); vTaskDelay(100);
            }
            break;
        
        case LED_STATE_IDLE:

            int64_t now;
            time_manager_get_timestamp(&now);
            if (now % 5 == 0) { // Blink for 100ms every 5 seconds
                ESP_LOGI("LED", "LED_STATE_IDLE"); //Blink every 5 seconds to indicate idle state
                led_on();
                vTaskDelay(1000); // Keep LED on for 1 second to make it more noticeable
            } else {
                led_off();
            }
            led_off(); // Ensure LED is off in idle state
            break;

        case LED_STATE_MEASURE:
            ESP_LOGI("LED", "LED_STATE_MEASURE");
            led_on(); vTaskDelay(50);
            led_off();
            break;

        case LED_STATE_UPLOAD:
            ESP_LOGI("LED", "LED_STATE_UPLOAD");
            for (int i=0;i<5;i++) {
                led_on(); vTaskDelay(50);
                led_off(); vTaskDelay(50);
            }
            break;

        case LED_STATE_SUCCESS:
            ESP_LOGI("LED", "LED_STATE_SUCCESS");
            led_on(); vTaskDelay(100);
            led_off(); vTaskDelay(100);
            led_on(); vTaskDelay(100);
            led_off();
            break;

        case LED_STATE_ERROR:
            ESP_LOGI("LED", "LED_STATE_ERROR");
            led_on(); vTaskDelay(500);
            led_off();
            break;
    }
}
