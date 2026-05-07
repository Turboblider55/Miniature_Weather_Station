#include "bh1750.h"
#include "esp_log.h"

#define BH1750_ADDR 0x23
#define BH1750_CMD_CONT_HIRES 0x10

// Initialize BH1750 sensor
esp_err_t bh1750_init(bh1750_handle_t *handle, i2c_master_bus_handle_t bus)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &handle->dev_handle));

    uint8_t cmd = BH1750_CMD_CONT_HIRES;

    return i2c_master_transmit(handle->dev_handle, &cmd, 1, -1);
}

// Read light level in lux
esp_err_t bh1750_read_lux(bh1750_handle_t *handle, float *lux)
{
    uint8_t data[2];

    esp_err_t err = i2c_master_receive(handle->dev_handle, data, 2, -1);
    if (err != ESP_OK) return err;

    uint16_t raw = (data[0] << 8) | data[1];

    // Convert raw value to lux (per BH1750 datasheet, divide by 1.2 for high-res mode)
    *lux = raw / 1.2f;

    return ESP_OK;
}