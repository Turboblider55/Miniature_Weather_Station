#include "display.h"
#include <string.h>
#include <ssd1306.h>
#include <math.h>
#include "wifi_manager.h"
#include <time.h>
#include "time_manager.h"
#include "upload_manager.h"
#include "measurement.h"
#include "cloudiness.h"

#include "small_text.h"

// Global variable to track current display page
int current_page = 0;
const int TOTAL_PAGES = 7;

const uint8_t icon_sun_32[128] = {
    0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,
    0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,
    0x0c,0x02,0x40,0x30,0x0f,0x82,0x41,0xf0,
    0x04,0xff,0xff,0x20,0x04,0x18,0x18,0x20,

    0x06,0x60,0x06,0x60,0x02,0x80,0x01,0x40,
    0x02,0x80,0x01,0x40,0x03,0x00,0x00,0xc0,
    0x03,0x00,0x00,0xc0,0x02,0x00,0x00,0x40,
    0x0e,0x00,0x00,0x70,0x72,0x01,0x80,0x4e,

    0x72,0x01,0x80,0x4e,0x0e,0x00,0x00,0x70,
    0x02,0x00,0x00,0x40,0x03,0x00,0x00,0xc0,
    0x03,0x00,0x00,0xc0,0x02,0x80,0x01,0x40,
    0x02,0x80,0x01,0x40,0x04,0x60,0x06,0x60,

    0x04,0x18,0x18,0x20,0x04,0xff,0xff,0x20,
    0x0f,0x82,0x41,0xf0,0x0c,0x02,0x40,0x30,
    0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,
    0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00
};

const uint8_t icon_cloud_32[128] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0xfc,0x00,0x00,

    0x0c,0x06,0x00,0x00,0x18,0x03,0x00,0x00,
    0x10,0x03,0xf8,0x00,0x10,0x01,0x0c,0x00,
    0x10,0x07,0x06,0x00,0x10,0x02,0x03,0x00,
    0x08,0x02,0x01,0x00,0x0c,0x00,0x03,0xe0,

    0x04,0x00,0x06,0x30,0x3e,0x00,0x0c,0x10,
    0x67,0x00,0x78,0x10,0x42,0x00,0x18,0x10,
    0xc2,0x00,0x08,0x30,0x80,0x10,0x00,0x78,
    0x80,0x1c,0x00,0x24,0x80,0x70,0x00,0x26,

    0xc0,0x40,0x00,0x02,0x60,0x40,0x00,0x03,
    0x38,0x60,0x00,0x01,0x07,0xff,0xff,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const uint8_t icon_partly_cloud_32[128] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,
    0x00,0x80,0x00,0x00,0x00,0x82,0x00,0x00,
    0x40,0x04,0x00,0x00,0x20,0x08,0x1f,0x00,

    0x13,0xe0,0x71,0x80,0x07,0xf0,0xc0,0xc0,
    0x0f,0xf1,0x80,0x60,0xef,0xf1,0x00,0x20,
    0x0f,0x03,0x00,0x20,0x0e,0x7f,0x00,0x10,
    0x06,0xc1,0x00,0x10,0x10,0x80,0x00,0x38,

    0x20,0x80,0x00,0x2c,0x41,0x80,0x00,0x66,
    0x01,0x00,0x00,0x03,0x1d,0x00,0x00,0x01,
    0x33,0x00,0x00,0x01,0x21,0x80,0x00,0x03,
    0x20,0x00,0x00,0x06,0x30,0x00,0x00,0x0c,

    0x18,0x00,0x00,0x38,0x0f,0xff,0xff,0xe0,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const uint8_t icon_moon_32[128] = {
    0x00,0x00,0x38,0x00,0x00,0x00,0x1e,0x00,
    0x00,0x20,0x0f,0x80,0x00,0x20,0x07,0xc0,
    0x00,0x00,0x07,0xe0,0x00,0x20,0x03,0xf0,
    0x00,0x20,0x03,0xf8,0x00,0x70,0x03,0xfc,

    0x00,0xf8,0x01,0xfc,0x01,0xfc,0x01,0xfe,
    0x37,0xdf,0x61,0xfe,0x01,0xfc,0x01,0xfe,
    0x00,0xf8,0x01,0xff,0x00,0x70,0x01,0xff,
    0x00,0x20,0x03,0xff,0x00,0x20,0x03,0xff,

    0x00,0x00,0x03,0xff,0x00,0x20,0x07,0xff,
    0x80,0x20,0x07,0xff,0xc0,0x00,0x0f,0xff,
    0xe0,0x00,0x1f,0xfe,0x78,0x00,0x7f,0xfe,
    0x7f,0x03,0xff,0xfe,0x3f,0xff,0xff,0xfc,

    0x3f,0xff,0xff,0xfc,0x1f,0xff,0xff,0xf8,
    0x0f,0xff,0xff,0xf0,0x07,0xff,0xff,0xe0,
    0x03,0xff,0xff,0xc0,0x01,0xff,0xff,0x80,
    0x00,0x7f,0xfe,0x00,0x00,0x0f,0xf0,0x00
};

// Helper function to convert upload result enum to text for display
static const char *upload_result_to_text(upload_result_t r)
{
    switch (r) {
        case UPLOAD_OK:           return "UPLOAD OK";
        case UPLOAD_NET_ERROR:    return "NET ERROR";
        case UPLOAD_AUTH_ERROR:   return "AUTH ERROR";
        case UPLOAD_SERVER_ERROR: return "SERVER ERROR";
        case UPLOAD_SKIPPED:      return "NO UPLOAD";
        default:                  return "UPLOAD FAIL";
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
    int gauge_x = 55;  // Center of gauge
    int gauge_y = 40;   // Center vertically

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
    display_small_text(ssd1306_handle, "900hPa", 75, 20);   // Left side of gauge
    display_small_text(ssd1306_handle, "1000hPa", 75, 36);  // Bottom of gauge
    display_small_text(ssd1306_handle, "1100hPa", 75, 52);  // Right side of gauge

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
    int Rad = 12;

    // Water drop shape
    ssd1306_set_circle(ssd1306_handle, drop_x, drop_y + Rad, Rad, false);  // Bottom circle
    //Clear top half of the circle to create drop shape
    for (int y = drop_y; y < drop_y + Rad; y++) {
        ssd1306_set_line(ssd1306_handle, drop_x - Rad, y, drop_x + Rad, y, true);
    }
    ssd1306_set_rectangle(ssd1306_handle, drop_x - Rad, drop_y - Rad, 2 * Rad, 2 * Rad , true); // Clear top of the circle to create drop shape
    
    ssd1306_set_line(ssd1306_handle, drop_x - Rad, drop_y + Rad, drop_x , drop_y - Rad, false);  // Left curve
    ssd1306_set_line(ssd1306_handle, drop_x + Rad, drop_y + Rad, drop_x , drop_y - Rad, false);  // Right curve
    //ssd1306_set_line(ssd1306_handle, drop_x - 4, drop_y - 5, drop_x + 4, drop_y - 5, false);  // Top

    // Fill drop based on humidity level
    int fill_levels = (int)(humidity / 6.6f);  // 0-15 levels
    if (fill_levels > 15) fill_levels = 15;
    if (fill_levels < 0) fill_levels = 0;

    for (int i = 0; i < fill_levels; i++) {
        int y = drop_y + (Rad - 1) * 2 - i * 2;
        if (y > drop_y + Rad) {
            ssd1306_set_line(ssd1306_handle, drop_x - (Rad)/2 - i, y, drop_x + (Rad)/2 + i, y, false);
        }
        else if(y >= (drop_y + Rad - 1)  && y <= (drop_y + Rad + 1)) {
            ssd1306_set_line(ssd1306_handle, drop_x - (Rad - 2) , y, drop_x + (Rad - 2) , y, false);
        }
        else {
            ssd1306_set_line(ssd1306_handle, drop_x - (Rad)/2 + (i - Rad / 2 - 3) , y, drop_x + (Rad)/2 - (i - Rad / 2 - 3) , y, false);
        }
    }

    // Humidity scale labels
    display_small_text(ssd1306_handle, "100%", 100, 25);  // Top
    ssd1306_set_line(ssd1306_handle, drop_x + Rad, 27, 97, 27, false); // Top scale line
    display_small_text(ssd1306_handle, "50%", 100, 42);   // Middle
    ssd1306_set_line(ssd1306_handle, drop_x + Rad + 5, 45, 97, 45, false); // Middle scale line
    display_small_text(ssd1306_handle, "0%", 100, 55);    // Bottom
    ssd1306_set_line(ssd1306_handle, drop_x + Rad, 57, 97, 57, false); // Bottom scale line

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


void display_wifi_connecting_page(
    const char *ssid,
    int attempt,
    int max_attempts)
{
    ssd1306_clear_display(ssd1306_handle, false);

    ssd1306_display_text(
        ssd1306_handle, 0,
        "WIFI CONNECTING", false);

    char line1[32];
    snprintf(line1, sizeof(line1),
             "SSID: %s", ssid);
    ssd1306_display_text(
        ssd1306_handle, 2,
        line1, false);

    char line2[32];
    snprintf(line2, sizeof(line2),
             "Attempt %d / %d",
             attempt, max_attempts);
    ssd1306_display_text(
        ssd1306_handle, 4,
        line2, false);

    // Simple animated dots
    int dots = attempt % 4;
    char anim[8] = ".";
    for (int i = 0; i < dots; i++) strcat(anim, ".");
    ssd1306_display_text(
        ssd1306_handle, 6,
        anim, false);

    ssd1306_display_pages(ssd1306_handle);
}

// Helper function to convert WiFi RSSI to signal bars (0-3)
static int wifi_rssi_to_bars(int rssi)
{
    if (rssi > -55) return 3;
    if (rssi > -70) return 2;
    if (rssi > -85) return 1;
    return 0;
}

// Function to draw WiFi signal strength icon based on RSSI bars
static void draw_wifi_icon(int x, int y, int bars)
{
    //Instead of drawing traditional Wifi arcs, we use rectangles to represent signal strength for better visibility on small display

    // Draw signal strength bars (3 bars total)
    ssd1306_set_rectangle(ssd1306_handle, x , y - 5, 2, 5, false); 
    ssd1306_set_rectangle(ssd1306_handle, x + 5, y - 10, 2, 10, false); 
    ssd1306_set_rectangle(ssd1306_handle, x + 10, y - 15, 2, 15, false);

    if (bars >= 1)
        //ssd1306_set_circle(ssd1306_handle, x, y, 2, false);
        ssd1306_set_line(ssd1306_handle, x + 1, y - 5, x + 1, y, false); // Small bar for 1 bar signal
    if (bars >= 2)
        //ssd1306_set_circle(ssd1306_handle, x, y, 5, false);
        ssd1306_set_line(ssd1306_handle, x + 6, y - 10, x + 6, y, false); // Medium bar for 2 bars signal
    if (bars >= 3)
        //ssd1306_set_circle(ssd1306_handle, x, y, 8, false);
        ssd1306_set_line(ssd1306_handle, x + 11, y - 15, x + 11, y, false); // Large bar for 3 bars signal
}

// Function to display WiFi connected page with SSID and signal strength
void display_wifi_connected_page(
    const char *ssid,
    int rssi)
{
    ssd1306_clear_display(ssd1306_handle, false);

    ssd1306_display_text(
        ssd1306_handle, 0,
        "WIFI CONNECTED", false);

    char line1[32];
    snprintf(line1, sizeof(line1),
             "SSID: %s", ssid);
    ssd1306_display_text(
        ssd1306_handle, 2,
        line1, false);

    int bars = wifi_rssi_to_bars(rssi);
    draw_wifi_icon(96, 45, bars);

    char line2[16];
    snprintf(line2, sizeof(line2),
             "Signal: %d/3", bars);
    ssd1306_display_text(
        ssd1306_handle, 5,
        line2, false);

    ssd1306_display_pages(ssd1306_handle);
}

// Function to display time and date page with large time and date
void display_time_date_page(void)
{
    ssd1306_clear_display(ssd1306_handle, false);

    int64_t timestamp;
    if (!time_manager_get_timestamp(&timestamp)) {
        // Time not available yet
        ssd1306_display_text(
            ssd1306_handle, 2,
            "TIME NOT READY", false);
        ssd1306_display_text(
            ssd1306_handle, 4,
            "WAITING FOR NTP", false);
        ssd1306_display_pages(ssd1306_handle);
        return;
    }

    time_t now = (time_t)timestamp;
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // ---- Large time (HH:MM) ----
    char time_str[8];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);

    // Draw large time manually using existing text function
    ssd1306_display_text(ssd1306_handle, 0, time_str, false);

    // ---- Day name ----
    static const char *days[] = {
        "Sunday", "Monday", "Tuesday",
        "Wednesday", "Thursday", "Friday", "Saturday"
    };

    ssd1306_display_text(
        ssd1306_handle, 2,
        days[timeinfo.tm_wday], false);

    // ---- Date (DD/MM/YYYY) ----
    char date_str[24];
    strftime(date_str, sizeof(date_str), "%d/%m/%Y", &timeinfo);

    ssd1306_display_text(
        ssd1306_handle, 4,
        date_str, false);

    // Optional subtle clock icon (simple)
    ssd1306_set_circle(ssd1306_handle, 110, 12, 6, false);
    ssd1306_set_line(
        ssd1306_handle,
        110, 12,
        110, 8,
        false
    );
    ssd1306_set_line(
        ssd1306_handle,
        110, 12,
        113, 12,
        false
    );

    ssd1306_display_pages(ssd1306_handle);
}

void display_upload_status_page(void)
{
    ssd1306_clear_display(ssd1306_handle, false);

    upload_result_t result = upload_manager_get_last_result();
    const char *status_text = upload_result_to_text(result);

    ssd1306_display_text(ssd1306_handle, 0, "UPLOAD STATUS", false);
    ssd1306_display_text(ssd1306_handle, 2, status_text, false);

    char buf[32];
    snprintf(buf, sizeof(buf), "Queued: %d", measurement_count());
    ssd1306_display_text(ssd1306_handle, 4, buf, false);

    ssd1306_display_pages(ssd1306_handle);
}

void display_light_page(int32_t lux)
{
    ssd1306_clear_display(ssd1306_handle, false);
    
    char buf[32];
    snprintf(buf, sizeof(buf), "Light: %ld lux", lux);

    float cloud_index = calculate_cloud_index((float)lux,CLOUDINESS_REFERENCE_LUX);

    char cloud_str[16];
    strcpy(cloud_str, cloud_label(cloud_index));
    
    int icon_index = cloud_index_to_icon(cloud_index);
    const uint8_t *to_display_icon; 

    if(icon_index == 0) {
        to_display_icon = icon_sun_32;
    } else if(icon_index == 1) {
        //memcpy(to_display_icon, icon_partly_cloud_32, sizeof(icon_partly_cloud_32));
        to_display_icon = icon_partly_cloud_32;
    } else if(icon_index == 2) {
        //memcpy(to_display_icon, icon_cloud_32, sizeof(icon_cloud_32));
        to_display_icon = icon_cloud_32;
    } else {
        //memcpy(to_display_icon, icon_moon_32, sizeof(icon_moon_32));
        to_display_icon = icon_moon_32;
    }

    cloud_index = 100.0f - (cloud_index * 100.0f); // Invert cloud index so that higher values mean clearer skies

    ssd1306_display_text(ssd1306_handle, 0, buf, false);
    ssd1306_display_text(ssd1306_handle, 4, cloud_str, false);
    ssd1306_set_bitmap(ssd1306_handle, 80, 24, to_display_icon, 32, 32, false);
    ssd1306_display_pages(ssd1306_handle);
}

void display_sensor_data_pages(
    float temperature,
    float humidity,
    float pressure,
    float altitude,
    int32_t lux
        )
    {

        switch (current_page) {
            case 0:
                // Time and date page
                display_time_date_page();
                break;
            case 1:
                // Temperature page (AHT21 or BMP280 temperature)
                display_temperature_page(temperature);
                break;

            case 2:
                // Humidity page (AHT21)
                display_humidity_page(humidity);
                break;

            case 3:
                // Pressure page (BMP280)
                display_pressure_page(pressure);
                break;

            case 4:
                // Altitude page (BMP280)
                display_altitude_page(altitude);
                break;

            case 5:
                if (!wifi_manager_is_connected()) {
                    display_wifi_connecting_page(
                        (char*)wifi_manager_get_ssid(),
                        wifi_manager_get_retry_count(),
                        wifi_manager_get_max_retry());
                }
                else {
                    display_wifi_connected_page(
                    (char*)wifi_manager_get_ssid(),
                    wifi_manager_get_rssi());
                }
                break;

             case 6:
                display_light_page(lux);
                break;

            /*
            // ENS160 PAGES DISABLED
            case 7:
                display_tvoc_page(tvoc);
                break;
            case 8:
                display_eco2_page(eco2);
                break;
            */
        }

        // Cycle to next page
        current_page = (current_page + 1) % TOTAL_PAGES;
    }
