#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>
#include <driver/i2c_master.h>

// BMP280 I2C Address
#define BMP280_I2C_ADDR 0x76

// BMP280 Registers
#define BMP280_REG_ID                  0xD0
#define BMP280_REG_RESET               0xE0
#define BMP280_REG_STATUS              0xF3
#define BMP280_REG_CTRL_MEAS           0xF4
#define BMP280_REG_CONFIG              0xF5
#define BMP280_REG_PRESS_MSB           0xF7
#define BMP280_REG_CALIB_START         0x88
#define BMP280_REG_CALIB_LEN           26

// BMP280 Structure for calibration data
typedef struct {
    uint16_t T1;
    int16_t T2;
    int16_t T3;
    uint16_t P1;
    int16_t P2;
    int16_t P3;
    int16_t P4;
    int16_t P5;
    int16_t P6;
    int16_t P7;
    int16_t P8;
    int16_t P9;
} bmp280_calib_t;

typedef struct {
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    bmp280_calib_t calib;
} bmp280_handle_t;

// Function declarations
esp_err_t bmp280_init(bmp280_handle_t *handle, i2c_master_bus_handle_t bus_handle);
esp_err_t bmp280_read_calibration(bmp280_handle_t *handle);
esp_err_t bmp280_read_raw_data(bmp280_handle_t *handle, int32_t *temperature, int32_t *pressure);
esp_err_t bmp280_compensate_temperature(bmp280_handle_t *handle, int32_t adc_T, int32_t *temperature);
esp_err_t bmp280_compensate_pressure(bmp280_handle_t *handle, int32_t adc_P, int32_t adc_T, int32_t *pressure);
esp_err_t bmp280_read_compensated_data(bmp280_handle_t *handle, float *temperature, float *pressure, float *altitude);

#endif // BMP280_H