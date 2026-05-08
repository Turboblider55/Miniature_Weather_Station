#include "cloudiness.h"

float calculate_cloud_index(float lux, float reference_lux)
{
    if (reference_lux <= 0.1f) return 0.0f;

    float index = lux / reference_lux;

    if (index > 1.0f) index = 1.0f;
    if (index < 0.0f) index = 0.0f;

    return index;
}

weather_icon_t cloud_index_to_icon(float index)
{
    if (index > 0.7f) return ICON_SUN; // sunny
    if (index > 0.5f) return ICON_CLOUD; // cloudy
    if (index > 0.3f) return ICON_PARTLY_CLOUD; // partly cloudy
    return ICON_MOON; // overcast or very low light (night)
}

const char *cloud_label(float index)
{
    if (index > 0.7f) return "Clear";
    if (index > 0.5f) return "Cloudy";
    if (index > 0.3f) return "Partly Cloudy";
    return "Overcast";
}
