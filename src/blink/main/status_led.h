#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"    
#include "esp_log.h"
#include "time_manager.h"

#define LED_GPIO 15   // change to your onboard LED

typedef enum {
    LED_STATE_BOOT,
    LED_STATE_IDLE,
    LED_STATE_MEASURE,
    LED_STATE_UPLOAD,
    LED_STATE_SUCCESS,
    LED_STATE_ERROR,
} led_state_t;


void status_led_init(void);
void led_on(void);
void led_off(void);
void status_led_set(led_state_t state);
