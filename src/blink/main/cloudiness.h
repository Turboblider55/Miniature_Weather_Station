#pragma once
typedef enum {
    ICON_SUN,
    ICON_PARTLY_CLOUD,
    ICON_CLOUD,
    ICON_MOON
} weather_icon_t;

#define CLOUDINESS_REFERENCE_LUX 10000.0f // Example reference lux for clear sky

float calculate_cloud_index(float lux, float reference_lux);
weather_icon_t cloud_index_to_icon(float index);
const char *cloud_label(float index);