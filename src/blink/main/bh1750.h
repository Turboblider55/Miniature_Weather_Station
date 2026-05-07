#pragma once
#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct {
    i2c_master_dev_handle_t dev_handle;
} bh1750_handle_t;

esp_err_t bh1750_init(bh1750_handle_t *handle, i2c_master_bus_handle_t bus);
esp_err_t bh1750_read_lux(bh1750_handle_t *handle, float *lux);