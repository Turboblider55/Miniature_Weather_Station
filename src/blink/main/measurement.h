#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <stdint.h>
#include <stdbool.h>

#define MEASUREMENT_NAMESPACE "measurements"
#define MEASUREMENT_MAX_COUNT 64   // configurable safety limit

typedef struct {
    int64_t timestamp_utc;     // seconds since epoch (UTC)

    int16_t temperature_c_x100;
    int16_t humidity_x100;
    int32_t pressure_hpa_x100;
    int32_t altitude_m_x10;

    int32_t lux; // future BH1750 field (fixed-point representation, nullable)
    uint16_t cloud_index; // future field for cloudiness index (0-100, fixed-point representation)

    // future ENS160 fields (nullable)
    int16_t tvoc_ppb;
    int16_t eco2_ppm;

} measurement_t;

/* API */
bool measurement_store(const measurement_t *m);
int  measurement_count(void);
bool measurement_get(uint32_t index, measurement_t *out);
bool measurement_delete(uint32_t count);
void measurement_clear_all(void);

#endif