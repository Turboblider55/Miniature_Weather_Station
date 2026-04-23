#include "ens160_aht21.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "ENS160_AHT21";

// I2C address scanner
void i2c_scan_devices(i2c_master_bus_handle_t bus_handle) {
    ESP_LOGI(TAG, "Scanning I2C bus for devices...");
    int found_count = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        i2c_device_config_t dev_config = {
            .device_address = addr,
            .scl_speed_hz = 100 * 1000,
        };
        i2c_master_dev_handle_t dev_handle;
        esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
        if (ret == ESP_OK) {
            uint8_t dummy;
            ret = i2c_master_transmit_receive(dev_handle, NULL, 0, &dummy, 1, 100);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Found device at address 0x%02X", addr);
                found_count++;
            }
            i2c_master_bus_rm_device(dev_handle);
        }
    }
    if (found_count == 0) {
        ESP_LOGW(TAG, "No I2C devices detected on this bus");
    } else {
        ESP_LOGI(TAG, "I2C scan complete: %d device(s) detected", found_count);
    }
}

// Initialize ENS160 and AHT21 sensors
esp_err_t ens160_aht21_init(ens160_aht21_handle_t *handle, i2c_master_bus_handle_t bus_handle)
{
    handle->bus_handle = bus_handle;

    // Scan I2C bus for debugging
    //i2c_scan_devices(bus_handle);

    // Initialize AHT21 device first (more reliable)
    i2c_device_config_t aht21_dev_config = {
        .device_address = AHT21_I2C_ADDR,
        .scl_speed_hz = 100 * 1000, // 100kHz
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &aht21_dev_config, &handle->aht21_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AHT21 device to I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Test AHT21 communication by reading its status register
    uint8_t aht21_status;
    ret = i2c_master_transmit_receive(handle->aht21_dev_handle,
                                     (uint8_t[]){AHT21_REG_STATUS}, 1,
                                     &aht21_status, 1, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AHT21 not responding: %s", esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "AHT21 found at address 0x%02X, status=0x%02X", AHT21_I2C_ADDR, aht21_status);
    }

    // Initialize AHT21
    ret = i2c_master_transmit(handle->aht21_dev_handle,
                             (uint8_t[]){AHT21_CMD_INIT, 0x08, 0x00}, 3, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AHT21: %s", esp_err_to_name(ret));
        return ret;
    }
    else {
        ESP_LOGI(TAG, "AHT21 initialization command sent successfully");
    }

    // Wait for AHT21 to be ready (status bit 7 should be 0)
    // This can take up to 300ms according to the datasheet, so we wait a bit longer
    vTaskDelay(pdMS_TO_TICKS(500));
    //vTaskDelay(pdMS_TO_TICKS(100));

    // Try to initialize ENS160 at both possible addresses
    ESP_LOGI(TAG, "Attempting to initialize ENS160 sensor...");
    uint8_t ens160_addresses[] = {ENS160_I2C_ADDR_0, ENS160_I2C_ADDR_1};
    bool ens160_found = false;
    ESP_LOGI(TAG, "Trying ENS160 at address 0x%02X and 0x%02X", ENS160_I2C_ADDR_0, ENS160_I2C_ADDR_1);
    for (int i = 0; i < 2; i++) {
        uint8_t addr = ens160_addresses[i];
        ESP_LOGI(TAG, "Trying ENS160 at address 0x%02X", addr);

        i2c_device_config_t ens160_dev_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = 100 * 1000, // 100kHz
        };

        ret = i2c_master_bus_add_device(bus_handle, &ens160_dev_config, &handle->ens160_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to add ENS160 device at 0x%02X: %s", addr, esp_err_to_name(ret));
            continue;
        }
        ESP_LOGI(TAG, "ENS160 device added at address 0x%02X, testing communication...", addr);

        ret = i2c_master_transmit(
            handle->ens160_dev_handle,
            (uint8_t[]){ ENS160_REG_OPMODE, ENS160_OPMODE_IDLE },
            2,
            -1
        );

        ESP_LOGI(TAG, "ENS160 set to IDLE: %s", esp_err_to_name(ret));

        vTaskDelay(pdMS_TO_TICKS(10));

        uint8_t status;
        ret = i2c_master_transmit_receive(
            handle->ens160_dev_handle,
            (uint8_t[]){ ENS160_REG_DEVICE_STATUS },
            1,
            &status,
            1,
            -1
        );

        ESP_LOGI(TAG, "ENS160 DEVICE_STATUS = 0x%02X (%s)",
                status, esp_err_to_name(ret));

        // Check ENS160 device ID
        ESP_LOGI(TAG, "Reading ENS160 PART_ID register at address 0x%02X", addr);
        
        uint8_t part_id_buf[2];

        ret = i2c_master_transmit_receive(
            handle->ens160_dev_handle,
            (uint8_t[]){ ENS160_REG_PART_ID },
            1,
            part_id_buf,
            2,
            -1
        );

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read ENS160 PART_ID at 0x%02X: %s",
                    addr, esp_err_to_name(ret));
            i2c_master_bus_rm_device(handle->ens160_dev_handle);
            continue;
        }

        uint16_t part_id = (part_id_buf[0] << 8) | part_id_buf[1];

        if (part_id != 0x0160) {
            ESP_LOGW(TAG, "Unexpected ENS160 PART_ID at 0x%02X: 0x%04X",
                    addr, part_id);
            i2c_master_bus_rm_device(handle->ens160_dev_handle);
            continue;
        }

        ret = i2c_master_transmit(
            handle->ens160_dev_handle,
            (uint8_t[]){ ENS160_REG_OPMODE, ENS160_OPMODE_STANDARD },
            2,
            -1
        );

        ESP_LOGI(TAG, "ENS160 set to STANDARD: %s", esp_err_to_name(ret));

        vTaskDelay(pdMS_TO_TICKS(20));

        ESP_LOGI(TAG, "ENS160 found at address 0x%02X, PART_ID: 0x%04X",addr, part_id);
        ens160_found = true;

        // Reset ENS160
        ret = i2c_master_transmit(handle->ens160_dev_handle,
                                 (uint8_t[]){ENS160_REG_OPMODE, ENS160_OPMODE_RESET}, 2, -1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reset ENS160: %s", esp_err_to_name(ret));
            i2c_master_bus_rm_device(handle->ens160_dev_handle);
            continue;
        }

        // Set ENS160 to standard operating mode
        vTaskDelay(pdMS_TO_TICKS(5));

        uint8_t opmode;
        ret = i2c_master_transmit_receive(
            handle->ens160_dev_handle,
            (uint8_t[]){ENS160_REG_OPMODE}, 1,
            &opmode, 1, -1);

        if (ret != ESP_OK || opmode != ENS160_OPMODE_STANDARD) {
            ESP_LOGE(TAG, "ENS160 failed to enter STANDARD mode (0x%02X)", opmode);
            i2c_master_bus_rm_device(handle->ens160_dev_handle);
            continue;
        }

        ESP_LOGI(TAG, "ENS160 entered STANDARD mode");


        break; // Success, exit the loop
    }

    if (!ens160_found) {
        ESP_LOGE(TAG, "ENS160 not found at any address");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ENS160 and AHT21 sensors initialized successfully");
    return ESP_OK;
}

// Read ENS160 data (TVOC and eCO2)
esp_err_t ens160_read_data(ens160_aht21_handle_t *handle, uint16_t *tvoc, uint16_t *eco2)
{
    uint8_t data[4];

    uint8_t status;
    i2c_master_transmit_receive(handle->ens160_dev_handle,
        (uint8_t[]){ENS160_REG_DATA_STATUS}, 1,
        &status, 1, -1);

    if (!(status & 0x01)) {
        ESP_LOGW(TAG, "ENS160 data not ready");
        // Not an error, just means we should try again later
        return ESP_ERR_INVALID_STATE;
    }

    // Read TVOC
    esp_err_t ret = i2c_master_transmit_receive(handle->ens160_dev_handle,
                                               (uint8_t[]){ENS160_REG_TVOC}, 1,
                                               data, 2, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ENS160 TVOC: %s", esp_err_to_name(ret));
        return ret;
    }
    *tvoc = (data[1] << 8) | data[0];

    // Read eCO2
    ret = i2c_master_transmit_receive(handle->ens160_dev_handle,
                                     (uint8_t[]){ENS160_REG_ECO2}, 1,
                                     data, 2, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ENS160 eCO2: %s", esp_err_to_name(ret));
        return ret;
    }
    *eco2 = (data[1] << 8) | data[0];

    return ESP_OK;
}

// Read AHT21 data (temperature and humidity)
esp_err_t aht21_read_data(ens160_aht21_handle_t *handle, float *temperature, float *humidity)
{
    // Trigger measurement
    esp_err_t ret = i2c_master_transmit(handle->aht21_dev_handle,
                                       (uint8_t[]){AHT21_CMD_MEASURE, 0x33, 0x00}, 3, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to trigger AHT21 measurement: %s", esp_err_to_name(ret));
        return ret;
    }

    // Wait for measurement to complete (typically 80ms)
    vTaskDelay(pdMS_TO_TICKS(80));

    // Read data after measurement by raw I2C receive
    uint8_t data[6];
    ret = i2c_master_receive(handle->aht21_dev_handle, data, 6, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AHT21 raw read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Check if data is ready (bit 7 of status should be 0)
    if (data[0] & 0x80) {
        ESP_LOGE(TAG, "AHT21 measurement not ready");
        return ESP_FAIL;
    }

    // Parse humidity (20 bits)
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    *humidity = (float)raw_humidity * 100.0f / 1048576.0f;

    // Parse temperature (20 bits)
    uint32_t raw_temperature = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    *temperature = (float)raw_temperature * 200.0f / 1048576.0f - 50.0f;

    return ESP_OK;
}

// Read all sensor data
esp_err_t ens160_aht21_read_all_data(ens160_aht21_handle_t *handle, float *temperature, float *humidity, uint16_t *tvoc, uint16_t *eco2)
{
    esp_err_t ret;

    // Read AHT21 data
    ret = aht21_read_data(handle, temperature, humidity);
    if (ret != ESP_OK) {
        return ret;
    }

    // Update ENS160 with temperature and humidity for better accuracy
    int16_t temp_int = (int16_t)(*temperature * 64.0f);
    int16_t hum_int = (int16_t)(*humidity * 512.0f);

    uint8_t temp_data[2] = {(uint8_t)(temp_int >> 8), (uint8_t)(temp_int & 0xFF)};
    ret = i2c_master_transmit(handle->ens160_dev_handle,
                             (uint8_t[]){ENS160_REG_TEMP_IN, temp_data[0], temp_data[1]}, 3, -1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to update ENS160 temperature: %s", esp_err_to_name(ret));
    }

    uint8_t hum_data[2] = {(uint8_t)(hum_int >> 8), (uint8_t)(hum_int & 0xFF)};
    ret = i2c_master_transmit(handle->ens160_dev_handle,
                             (uint8_t[]){ENS160_REG_RH_IN, hum_data[0], hum_data[1]}, 3, -1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to update ENS160 humidity: %s", esp_err_to_name(ret));
    }

    // Read ENS160 data
    ret = ens160_read_data(handle, tvoc, eco2);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}