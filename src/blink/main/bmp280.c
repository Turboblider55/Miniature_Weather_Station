#include "bmp280.h"
#include <esp_log.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "BMP280";

// Initialize BMP280 sensor
esp_err_t bmp280_init(bmp280_handle_t *handle, i2c_master_bus_handle_t bus_handle)
{
    handle->bus_handle = bus_handle;

    i2c_device_config_t i2c_dev_config = {
        .device_address = BMP280_I2C_ADDR,
        .scl_speed_hz = 100 * 1000, // 100kHz
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &i2c_dev_config, &handle->dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add BMP280 device to I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }
    else {
        ESP_LOGI(TAG, "BMP280 device added to I2C bus at address 0x%02X", BMP280_I2C_ADDR);
    }

    // Read and verify device ID
    uint8_t device_id;
    ret = i2c_master_transmit_receive(handle->dev_handle,
                                     (uint8_t[]){BMP280_REG_ID}, 1,
                                     &device_id, 1, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BMP280 ID: %s", esp_err_to_name(ret));
        return ret;
    }

    if (device_id != 0x58) {
        ESP_LOGE(TAG, "Unexpected BMP280 device ID: 0x%02X (expected 0x58)", device_id);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "BMP280 device ID verified: 0x%02X", device_id);
    }

    // Check status register
    uint8_t status;
    ret = i2c_master_transmit_receive(handle->dev_handle,
                                     (uint8_t[]){BMP280_REG_STATUS}, 1,
                                     &status, 1, -1);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "BMP280 status: 0x%02X", status);
    }

    // Read calibration data
    ret = bmp280_read_calibration(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BMP280 calibration data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Debug: Log calibration data
    ESP_LOGI(TAG, "BMP280 Calib: T1=%u T2=%d T3=%d P1=%u P2=%d P3=%d P4=%d P5=%d P6=%d P7=%d P8=%d P9=%d",
             handle->calib.T1, handle->calib.T2, handle->calib.T3,
             handle->calib.P1, handle->calib.P2, handle->calib.P3,
             handle->calib.P4, handle->calib.P5, handle->calib.P6,
             handle->calib.P7, handle->calib.P8, handle->calib.P9);

    // Configure BMP280: Normal mode, OSR settings
    // ctrl_meas: osrs_t=101 (16x), osrs_p=101 (16x), mode=11 (normal)
    uint8_t ctrl_meas = 0xB7;  // 10110111
    ret = i2c_master_transmit(handle->dev_handle,
                             (uint8_t[]){BMP280_REG_CTRL_MEAS, ctrl_meas}, 2, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure BMP280 ctrl_meas: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure BMP280: Normal mode standby, IIR filter
    // config: standby=101 (1000ms), filter=101 (16x), spi3w_en=0
    uint8_t config = 0xB0;  // 10110000
    ret = i2c_master_transmit(handle->dev_handle,
                             (uint8_t[]){BMP280_REG_CONFIG, config}, 2, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure BMP280 config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Wait for sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "BMP280 sensor initialized successfully");
    return ESP_OK;
}

// Read calibration data from BMP280
esp_err_t bmp280_read_calibration(bmp280_handle_t *handle)
{
    uint8_t calib_data[BMP280_REG_CALIB_LEN];
    esp_err_t ret = i2c_master_transmit_receive(handle->dev_handle,
                                               (uint8_t[]){BMP280_REG_CALIB_START}, 1,
                                               calib_data, BMP280_REG_CALIB_LEN, -1);
    if (ret != ESP_OK) {
        return ret;
    }

    handle->calib.T1 = (calib_data[1] << 8) | calib_data[0];
    handle->calib.T2 = (int16_t)((calib_data[3] << 8) | calib_data[2]);
    handle->calib.T3 = (int16_t)((calib_data[5] << 8) | calib_data[4]);
    handle->calib.P1 = (calib_data[7] << 8) | calib_data[6];
    handle->calib.P2 = (int16_t)((calib_data[9] << 8) | calib_data[8]);
    handle->calib.P3 = (int16_t)((calib_data[11] << 8) | calib_data[10]);
    handle->calib.P4 = (int16_t)((calib_data[13] << 8) | calib_data[12]);
    handle->calib.P5 = (int16_t)((calib_data[15] << 8) | calib_data[14]);
    handle->calib.P6 = (int16_t)((calib_data[17] << 8) | calib_data[16]);
    handle->calib.P7 = (int16_t)((calib_data[19] << 8) | calib_data[18]);
    handle->calib.P8 = (int16_t)((calib_data[21] << 8) | calib_data[20]);
    handle->calib.P9 = (int16_t)((calib_data[23] << 8) | calib_data[22]);

    return ESP_OK;
}

// Read raw temperature and pressure data
esp_err_t bmp280_read_raw_data(bmp280_handle_t *handle, int32_t *temperature, int32_t *pressure)
{
    uint8_t data[6];

    esp_err_t ret = i2c_master_transmit_receive(handle->dev_handle,
                                               (uint8_t[]){BMP280_REG_PRESS_MSB}, 1,
                                               data, 6, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BMP280 data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Debug: Log raw data bytes
    ESP_LOGD(TAG, "Raw data bytes: %02X %02X %02X %02X %02X %02X",
             data[0], data[1], data[2], data[3], data[4], data[5]);

    *pressure = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    *temperature = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    return ESP_OK;
}

// Compensate temperature reading
esp_err_t bmp280_compensate_temperature(bmp280_handle_t *handle, int32_t adc_T, int32_t *temperature)
{
    static int32_t t_fine; // Static to maintain state between calls

    int32_t var1, var2, T;
    var1 = ((adc_T >> 3) - ((int32_t)handle->calib.T1 << 1));
    var2 = (var1 * ((int32_t)handle->calib.T2)) >> 11;
    var1 = ((((adc_T >> 4) - ((int32_t)handle->calib.T1)) * (((adc_T >> 4) - ((int32_t)handle->calib.T1)) >> 12)) * ((int32_t)handle->calib.T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    *temperature = T;

    return ESP_OK;
}

// Compensate pressure reading
esp_err_t bmp280_compensate_pressure(bmp280_handle_t *handle, int32_t adc_P, int32_t adc_T, int32_t *pressure)
{
    static int32_t t_fine; // Static to maintain state between calls

    // First compensate temperature to get t_fine
    int32_t temp;
    bmp280_compensate_temperature(handle, adc_T, &temp);

    int32_t var1, var2;
    uint32_t p;

    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)handle->calib.P6);
    var2 = var2 + ((var1 * ((int32_t)handle->calib.P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)handle->calib.P4) << 16);
    var1 = (((handle->calib.P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)handle->calib.P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)handle->calib.P1)) >> 15);

    if (var1 == 0) {
        return ESP_FAIL; // avoid exception caused by division by zero
    }

    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;

    if (p < 0x80000000) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }

    var1 = (((int32_t)handle->calib.P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)handle->calib.P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + handle->calib.P7) >> 4));

    *pressure = (int32_t)p;

    return ESP_OK;
}

// Read and compensate all sensor data
esp_err_t bmp280_read_compensated_data(bmp280_handle_t *handle, float *temperature, float *pressure, float *altitude)
{
    int32_t adc_T, adc_P;
    int32_t temp_comp, press_comp;

    esp_err_t ret = bmp280_read_raw_data(handle, &adc_T, &adc_P);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = bmp280_compensate_temperature(handle, adc_T, &temp_comp);
    if (ret != ESP_OK) {
        return ret;
    }
    *temperature = temp_comp / 100.0f;

    ret = bmp280_compensate_pressure(handle, adc_P, adc_T, &press_comp);
    if (ret != ESP_OK) {
        return ret;
    }
    *pressure = press_comp / 100.0f;  // Convert Pa to hPa

    // Debug logging
    ESP_LOGD(TAG, "Raw ADC - Pressure: %ld, Temperature: %ld", adc_P, adc_T);
    ESP_LOGD(TAG, "Compensated - Pressure PA: %ld, Pressure hPa: %.2f, Temp: %.2f", press_comp, *pressure, *temperature);

    // Calculate altitude using barometric formula
    // Only calculate if we have reasonable pressure (between 300 and 1200 hPa)
    if (*pressure > 300.0f && *pressure < 1200.0f) {
        float sea_level_pressure = 1013.25f;
        float pressure_ratio = *pressure / sea_level_pressure;

        // altitude = 44330 * (1.0 - pow(pressure / sea_level_pressure, 1.0/5.255))
        *altitude = 44330.0f * (1.0f - powf(pressure_ratio, 0.190263f));  // 1/5.255 ≈ 0.190263
    } else {
        // If pressure is out of reasonable range, set altitude to 0
        ESP_LOGW(TAG, "Pressure out of range: %.2f hPa (ADC_P: %ld, PA: %ld)", *pressure, adc_P, press_comp);
        *altitude = 0.0f;
    }

    return ESP_OK;
}