#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <ssd1306.h>

// External OLED handle from main.c
extern ssd1306_handle_t ssd1306_handle;

// OLED Display Helper Functions
// Global variable to track current display page
extern int current_page;
extern const int TOTAL_PAGES;

// Small 6x6 font bitmaps for numbers 0-9 (6 pixels wide, 6 pixels high)
// Bit 5 = leftmost pixel, bit 0 = rightmost pixel
extern const uint8_t small_font_6x6[10][6];

// Function declarations
void display_small_number(ssd1306_handle_t handle, int number, int x, int y);
void display_small_text(ssd1306_handle_t handle, const char *text, int x, int y);
void display_temperature_page(float temperature);
void display_humidity_page(float humidity);
void display_tvoc_page(uint16_t tvoc);
void display_eco2_page(uint16_t eco2);
void display_sensor_data_pages(float temperature, float humidity, uint16_t tvoc, uint16_t eco2);

#endif // DISPLAY_H