#include "display.h"
#include <string.h>
#include <ssd1306.h>
#include <math.h>

// Global variable to track current display page
int current_page = 0;
const int TOTAL_PAGES = 4;

// Small 6x6 font bitmaps for numbers 0-9 (6 pixels wide, 6 pixels high)
// Bit 5 = leftmost pixel, bit 0 = rightmost pixel
const uint8_t small_font_6x6[10][6] = {
    {0x1E, 0x21, 0x21, 0x21, 0x21, 0x1E}, // 0: 011110, 100001, 100001, 100001, 100001, 011110
    {0x08, 0x18, 0x08, 0x08, 0x08, 0x1C}, // 1: 001000, 011000, 001000, 001000, 001000, 011100
    {0x1E, 0x01, 0x0E, 0x10, 0x20, 0x3F}, // 2: 011110, 000001, 001110, 010000, 100000, 111111
    {0x1E, 0x01, 0x0E, 0x01, 0x01, 0x1E}, // 3: 011110, 000001, 001110, 000001, 000001, 011110
    {0x11, 0x11, 0x1F, 0x01, 0x01, 0x01}, // 4: 010001, 010001, 011111, 000001, 000001, 000001
    {0x3F, 0x20, 0x3E, 0x01, 0x01, 0x3E}, // 5: 111111, 100000, 111110, 000001, 000001, 111110
    {0x1E, 0x20, 0x3E, 0x21, 0x21, 0x1E}, // 6: 011110, 100000, 111110, 100001, 100001, 011110
    {0x3F, 0x01, 0x02, 0x04, 0x08, 0x08}, // 7: 111111, 000001, 000010, 000100, 001000, 001000
    {0x1E, 0x21, 0x1E, 0x21, 0x21, 0x1E}, // 8: 011110, 100001, 011110, 100001, 100001, 011110
    {0x1E, 0x21, 0x1F, 0x01, 0x01, 0x1E}  // 9: 011110, 100001, 011111, 000001, 000001, 011110
};

// Function to display a small number at specific coordinates (6x6 font)
void display_small_number(ssd1306_handle_t handle, int number, int x, int y) {
    if (number < 0 || number > 9) return;

    for (int row = 0; row < 6; row++) {
        uint8_t bitmap_row = small_font_6x6[number][row];
        for (int col = 0; col < 6; col++) {
            if (bitmap_row & (1 << (5 - col))) {
                ssd1306_set_pixel(handle, x + col, y + row, false);
            }
        }
    }
}

// Function to display small text at specific coordinates
void display_small_text(ssd1306_handle_t handle, const char *text, int x, int y) {
    int current_x = x;
    while (*text) {
        if (*text >= '0' && *text <= '9') {
            display_small_number(handle, *text - '0', current_x, y);
            current_x += 7; // 6 pixels width + 1 pixel spacing
        } else if (*text == 'C') {
            // C character (4x6): 01110, 10000, 10000, 10000, 10000, 01110
            ssd1306_set_pixel(handle, current_x + 1, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 1, y + 5, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 5, false);
            current_x += 5;
        } else if (*text == 'h') {
            // h character (4x6): 10000, 10000, 11110, 10001, 10001, 10001
            ssd1306_set_pixel(handle, current_x + 0, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 5, false);
            ssd1306_set_pixel(handle, current_x + 1, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 5, false);
            current_x += 5;
        } else if (*text == 'P') {
            // P character (4x6): 11110, 10001, 10001, 11110, 10000, 10000
            ssd1306_set_pixel(handle, current_x + 0, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 1, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 5, false);
            ssd1306_set_pixel(handle, current_x + 1, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 1, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 1, false);
            current_x += 5;
        } else if (*text == 'a') {
            // a character (4x6): 01110, 10001, 10001, 01111, 00001, 01110
            ssd1306_set_pixel(handle, current_x + 1, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 3, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 1, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 3, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 3, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 3, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 5, false);
            ssd1306_set_pixel(handle, current_x + 3, y + 5, false);
            current_x += 5;
        } else if (*text == 'm') {
            // m character (6x6): 100001, 110011, 101101, 100001, 100001, 100001
            ssd1306_set_pixel(handle, current_x + 0, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 5, y + 0, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 1, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 3, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 4, y + 1, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 2, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 3, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 5, y + 2, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 5, y + 3, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 5, y + 4, false);
            ssd1306_set_pixel(handle, current_x + 0, y + 5, false);
            ssd1306_set_pixel(handle, current_x + 5, y + 5, false);
            current_x += 7;
        }
        text++;
    }
}

// Function to display temperature page with thermometer illustration
void display_temperature_page(float temperature)
{
    // Clear display
    ssd1306_clear_display(ssd1306_handle, false);

    // Draw thermometer illustration on the right side (x=60-80)
    int thermo_x = 70;  // Center of thermometer

    // Thermometer bulb
    ssd1306_set_circle(ssd1306_handle, thermo_x, 50, 6, false);
    ssd1306_set_circle(ssd1306_handle, thermo_x, 50, 4, true);  // Fill bulb

    // Thermometer tube
    ssd1306_set_line(ssd1306_handle, thermo_x - 2, 15, thermo_x - 2, 44, false);  // Left side
    ssd1306_set_line(ssd1306_handle, thermo_x + 2, 15, thermo_x + 2, 44, false);  // Right side
    ssd1306_set_line(ssd1306_handle, thermo_x - 2, 15, thermo_x + 2, 15, false);  // Top
    ssd1306_set_line(ssd1306_handle, thermo_x - 2, 44, thermo_x + 2, 44, false);  // Bottom

    // Temperature scale marks
    for (int i = 0; i <= 40; i += 10) {
        int y = 43 - (i * 28 / 40);  // Scale from 40C at top to 0C at bottom
        ssd1306_set_line(ssd1306_handle, thermo_x + 3, y, thermo_x + 6, y, false);
    }

    // Fill thermometer based on temperature (0-40C range)
    int fill_height = (int)((temperature / 40.0f) * 28.0f);
    if (fill_height > 28) fill_height = 28;
    if (fill_height < 0) fill_height = 0;

    for (int i = 0; i < fill_height; i++) {
        ssd1306_set_line(ssd1306_handle, thermo_x - 1, 43 - i, thermo_x + 1, 43 - i, false);
    }

    // Temperature scale labels on the left side of thermometer
    display_small_text(ssd1306_handle, "40C", 85, 15);  // Top of scale
    display_small_text(ssd1306_handle, "30C", 85, 23);  // Middle upper
    display_small_text(ssd1306_handle, "20C", 85, 31);  // Middle lower
    display_small_text(ssd1306_handle, "10C", 85, 39);  // Bottom upper
    display_small_text(ssd1306_handle, "0C", 85, 47);   // Bottom

    // Display temperature value and title on the left side
    ssd1306_display_text(ssd1306_handle, 0, "TEMPERATURE", false);
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.1f C", temperature);
    ssd1306_display_text(ssd1306_handle, 1, temp_str, false);

    // Update display
    ssd1306_display_pages(ssd1306_handle);
}

// Function to display pressure page with barometer illustration
void display_pressure_page(float pressure)
{
    // Clear display
    ssd1306_clear_display(ssd1306_handle, false);

    // Draw barometer illustration on the right side (x=70, y=35)
    int gauge_x = 65;  // Center of gauge
    int gauge_y = 35;   // Center vertically

    // Outer circle
    ssd1306_set_circle(ssd1306_handle, gauge_x, gauge_y, 18, false);

    // Inner circle
    ssd1306_set_circle(ssd1306_handle, gauge_x, gauge_y, 14, false);

    // Center dot
    ssd1306_set_pixel(ssd1306_handle, gauge_x, gauge_y, false);

    // Pressure scale marks
    for (int i = 900; i <= 1100; i += 50) {
        float angle = ((i - 900) / 200.0f) * 180.0f - 90.0f; // Map to -90 to +90 degrees
        float rad_angle = angle * 3.14159f / 180.0f;
        int x1 = gauge_x + (int)(12 * cosf(rad_angle));
        int y1 = gauge_y + (int)(12 * sinf(rad_angle));
        int x2 = gauge_x + (int)(16 * cosf(rad_angle));
        int y2 = gauge_y + (int)(16 * sinf(rad_angle));
        ssd1306_set_line(ssd1306_handle, x1, y1, x2, y2, false);
    }

    // Pressure needle (scaled 900-1100 hPa to -90 to +90 degrees)
    float pressure_normalized = (pressure - 900.0f) / 200.0f;
    if (pressure_normalized > 1.0f) pressure_normalized = 1.0f;
    if (pressure_normalized < 0.0f) pressure_normalized = 0.0f;

    float angle = (pressure_normalized * 180.0f - 90.0f) * 3.14159f / 180.0f; // -90 to +90 degrees
    int needle_x = gauge_x + (int)(11 * cosf(angle));
    int needle_y = gauge_y + (int)(11 * sinf(angle));
    ssd1306_set_line(ssd1306_handle, gauge_x, gauge_y, needle_x, needle_y, false);

    // Pressure scale labels on the left side of gauge
    display_small_text(ssd1306_handle, "900hPa", 85, 16);   // Left side of gauge
    display_small_text(ssd1306_handle, "1000hPa", 85, 32);  // Bottom of gauge
    display_small_text(ssd1306_handle, "1100hPa", 85, 48);  // Right side of gauge

    // Display pressure value and title on the left side
    ssd1306_display_text(ssd1306_handle, 0, "PRESSURE", false);
    char press_str[16];
    snprintf(press_str, sizeof(press_str), "%.1f hPa", pressure);
    ssd1306_display_text(ssd1306_handle, 1, press_str, false);

    // Update display
    ssd1306_display_pages(ssd1306_handle);
}

// Function to display altitude page with mountain illustration
void display_altitude_page(float altitude)
{
    // Clear display
    ssd1306_clear_display(ssd1306_handle, false);

    // Draw mountain illustration
    // Mountain peaks
    ssd1306_set_line(ssd1306_handle, 15, 50, 45, 30, false);  // Left peak
    ssd1306_set_line(ssd1306_handle, 45, 30, 50, 40, false);  // Middle slope
    ssd1306_set_line(ssd1306_handle, 50, 40, 70, 20, false);  // Right peak
    ssd1306_set_line(ssd1306_handle, 70, 20, 90, 35, false);  // Final slope
    ssd1306_set_line(ssd1306_handle, 90, 35, 110, 50, false); // Base

    // Fill mountain with diagonal lines
    for (int x = 20; x < 110; x += 3) {
        int y_start = 50 - ((x - 20) * 25 / 90); // Rough mountain height
        if (y_start < 20) y_start = 20;
        ssd1306_set_line(ssd1306_handle, x, 50, x, y_start, false);
    }

    // Altitude scale markers and labels
    // Add altitude labels on the left side using small custom font
    display_small_text(ssd1306_handle, "500m", 0, 16);  // Top
    display_small_text(ssd1306_handle, "250m", 0, 32);  // Middle
    display_small_text(ssd1306_handle, "0m", 0, 48);    // Bottom

    // Draw scale lines
    ssd1306_set_line(ssd1306_handle, 5, 20, 8, 20, false);   // 500m mark
    ssd1306_set_line(ssd1306_handle, 5, 35, 8, 35, false);   // 250m mark
    ssd1306_set_line(ssd1306_handle, 5, 50, 8, 50, false);   // 0m mark

    // Altitude indicator arrow
    float alt_normalized = altitude / 500.0f;
    if (alt_normalized > 1.0f) alt_normalized = 1.0f;
    if (alt_normalized < 0.0f) alt_normalized = 0.0f;

    int arrow_x = 10 + (int)(alt_normalized * 100.0f);
    int arrow_y = 45 - (int)(alt_normalized * 20.0f);

    // Arrow pointing up
    ssd1306_set_line(ssd1306_handle, arrow_x, arrow_y + 5, arrow_x, arrow_y, false);     // Shaft
    ssd1306_set_line(ssd1306_handle, arrow_x - 2, arrow_y + 2, arrow_x, arrow_y, false); // Left arrowhead
    ssd1306_set_line(ssd1306_handle, arrow_x + 2, arrow_y + 2, arrow_x, arrow_y, false); // Right arrowhead

    // Display altitude value
    char alt_str[16];
    snprintf(alt_str, sizeof(alt_str), "%.1f m", altitude);
    ssd1306_display_text(ssd1306_handle, 0, "ALTITUDE", false);
    ssd1306_display_text(ssd1306_handle, 1, alt_str, false);

    // Update display
    ssd1306_display_pages(ssd1306_handle);
}

// Function to display humidity page with water drop illustration
void display_humidity_page(float humidity)
{
    // Clear display
    ssd1306_clear_display(ssd1306_handle, false);

    // Draw water drop illustration on the right side
    int drop_x = 75;  // Center of drop
    int drop_y = 35;  // Center vertically

    // Water drop shape
    ssd1306_set_circle(ssd1306_handle, drop_x, drop_y + 8, 8, false);  // Bottom circle
    ssd1306_set_line(ssd1306_handle, drop_x - 8, drop_y + 8, drop_x - 4, drop_y - 5, false);  // Left curve
    ssd1306_set_line(ssd1306_handle, drop_x + 8, drop_y + 8, drop_x + 4, drop_y - 5, false);  // Right curve
    ssd1306_set_line(ssd1306_handle, drop_x - 4, drop_y - 5, drop_x + 4, drop_y - 5, false);  // Top

    // Fill drop based on humidity level
    int fill_levels = (int)(humidity / 10.0f);  // 0-10 levels
    if (fill_levels > 10) fill_levels = 10;
    if (fill_levels < 0) fill_levels = 0;

    for (int i = 0; i < fill_levels; i++) {
        int y = drop_y + 8 - i * 2;
        if (y > drop_y - 5) {
            ssd1306_set_line(ssd1306_handle, drop_x - 6 + i, y, drop_x + 6 - i, y, false);
        }
    }

    // Humidity scale labels
    display_small_text(ssd1306_handle, "100%", 85, 16);  // Top
    display_small_text(ssd1306_handle, "50%", 85, 32);   // Middle
    display_small_text(ssd1306_handle, "0%", 85, 48);    // Bottom

    // Display humidity value and title
    ssd1306_display_text(ssd1306_handle, 0, "HUMIDITY", false);
    char hum_str[16];
    snprintf(hum_str, sizeof(hum_str), "%.1f %%", humidity);
    ssd1306_display_text(ssd1306_handle, 1, hum_str, false);

    // Update display
    ssd1306_display_pages(ssd1306_handle);
}

// Function to display TVOC page with air quality illustration
void display_tvoc_page(uint16_t tvoc)
{
    // Clear display
    ssd1306_clear_display(ssd1306_handle, false);

    // Draw air quality illustration (simple cloud)
    int cloud_x = 70;
    int cloud_y = 35;

    // Cloud shape
    ssd1306_set_circle(ssd1306_handle, cloud_x - 8, cloud_y - 2, 6, false);
    ssd1306_set_circle(ssd1306_handle, cloud_x, cloud_y - 4, 8, false);
    ssd1306_set_circle(ssd1306_handle, cloud_x + 8, cloud_y - 2, 6, false);
    ssd1306_set_circle(ssd1306_handle, cloud_x, cloud_y + 4, 6, false);

    // TVOC level indicator (simple bar)
    float tvoc_level = tvoc / 1000.0f;  // Normalize to 0-1
    if (tvoc_level > 1.0f) tvoc_level = 1.0f;

    int bar_height = (int)(tvoc_level * 20);
    for (int i = 0; i < bar_height; i++) {
        ssd1306_set_line(ssd1306_handle, cloud_x - 2, cloud_y + 10 - i, cloud_x + 2, cloud_y + 10 - i, false);
    }

    // TVOC scale labels
    display_small_text(ssd1306_handle, "1000", 85, 16);  // High
    display_small_text(ssd1306_handle, "500", 85, 32);   // Medium
    display_small_text(ssd1306_handle, "0", 85, 48);     // Low

    // Display TVOC value and title
    ssd1306_display_text(ssd1306_handle, 0, "TVOC", false);
    char tvoc_str[16];
    snprintf(tvoc_str, sizeof(tvoc_str), "%d ppb", tvoc);
    ssd1306_display_text(ssd1306_handle, 1, tvoc_str, false);

    // Update display
    ssd1306_display_pages(ssd1306_handle);
}

// Function to display eCO2 page with CO2 molecule illustration
void display_eco2_page(uint16_t eco2)
{
    // Clear display
    ssd1306_clear_display(ssd1306_handle, false);

    // Draw CO2 molecule illustration (O=C=O)
    int center_x = 70;
    int center_y = 35;

    // Carbon atom (center)
    ssd1306_set_circle(ssd1306_handle, center_x, center_y, 3, false);

    // Oxygen atoms
    ssd1306_set_circle(ssd1306_handle, center_x - 12, center_y, 3, false);
    ssd1306_set_circle(ssd1306_handle, center_x + 12, center_y, 3, false);

    // Bonds
    ssd1306_set_line(ssd1306_handle, center_x - 8, center_y, center_x - 4, center_y, false);
    ssd1306_set_line(ssd1306_handle, center_x + 4, center_y, center_x + 8, center_y, false);

    // Double bonds (approximated)
    ssd1306_set_line(ssd1306_handle, center_x - 8, center_y - 1, center_x - 4, center_y - 1, false);
    ssd1306_set_line(ssd1306_handle, center_x + 4, center_y - 1, center_x + 8, center_y - 1, false);

    // eCO2 level indicator (simple bar)
    float eco2_level = (eco2 - 400.0f) / 1600.0f;  // 400-2000 ppm range
    if (eco2_level > 1.0f) eco2_level = 1.0f;
    if (eco2_level < 0.0f) eco2_level = 0.0f;

    int bar_height = (int)(eco2_level * 20);
    for (int i = 0; i < bar_height; i++) {
        ssd1306_set_line(ssd1306_handle, center_x - 2, center_y + 15 - i, center_x + 2, center_y + 15 - i, false);
    }

    // eCO2 scale labels
    display_small_text(ssd1306_handle, "2000", 85, 16);  // High
    display_small_text(ssd1306_handle, "1200", 85, 32);  // Medium
    display_small_text(ssd1306_handle, "400", 85, 48);   // Low

    // Display eCO2 value and title
    ssd1306_display_text(ssd1306_handle, 0, "eCO2", false);
    char eco2_str[16];
    snprintf(eco2_str, sizeof(eco2_str), "%d ppm", eco2);
    ssd1306_display_text(ssd1306_handle, 1, eco2_str, false);

    // Update display
    ssd1306_display_pages(ssd1306_handle);
}

// // Main display function that cycles through pages
// void display_sensor_data_pages(float temperature, float humidity, uint16_t tvoc, uint16_t eco2)
// {
//     switch (current_page) {
//         case 0:
//             display_temperature_page(temperature);
//             break;
//         case 1:
//             display_humidity_page(humidity);
//             break;
//         case 2:
//             display_tvoc_page(tvoc);
//             break;
//         case 3:
//             display_eco2_page(eco2);
//             break;
//     }

//     // Cycle to next page
//     current_page = (current_page + 1) % TOTAL_PAGES;
// }


void display_sensor_data_pages(
    float temperature,
    float humidity,
    float pressure,
    float altitude)
    {
        switch (current_page) {
            case 0:
                // Temperature page (AHT21 or BMP280 temperature)
                display_temperature_page(temperature);
                break;

            case 1:
                // Humidity page (AHT21)
                display_humidity_page(humidity);
                break;

            case 2:
                // Pressure page (BMP280)
                display_pressure_page(pressure);
                break;

            case 3:
                // Altitude page (BMP280)
                display_altitude_page(altitude);
                break;

            /*
            // ENS160 PAGES DISABLED
            case 2:
                display_tvoc_page(tvoc);
                break;
            case 3:
                display_eco2_page(eco2);
                break;
            */
        }

        // Cycle to next page
        current_page = (current_page + 1) % TOTAL_PAGES;
    }
