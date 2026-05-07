#include "cloudiness.h"

float calculate_cloud_index(float lux, float reference_lux)
{
    if (reference_lux <= 0.1f) return 0.0f;

    float index = lux / reference_lux;

    if (index > 1.0f) index = 1.0f;
    if (index < 0.0f) index = 0.0f;

    return index;
}

int cloud_index_to_icon(float index)
{
    if (index > 0.7f) return 0; // sunny
    if (index > 0.3f) return 1; // partly cloudy
    return 2; // cloudy
}