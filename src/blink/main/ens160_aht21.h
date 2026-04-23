#ifndef ENS160_AHT21_H
#define ENS160_AHT21_H

#include <stdint.h>
#include <driver/i2c_master.h>

// ENS160 I2C Address
#define ENS160_I2C_ADDR_0 0x53  // ADD tied to 3V3
#define ENS160_I2C_ADDR_1 0x52  // ADD tied to GND
#define ENS160_I2C_ADDR ENS160_I2C_ADDR_1  // Default to ADD=GND which is more common 

// AHT21-M I2C Address
#define AHT21_I2C_ADDR 0x38

// ENS160 Registers
#define ENS160_REG_PART_ID          0x00
#define ENS160_REG_OPMODE           0x10
#define ENS160_REG_CONFIG           0x11
#define ENS160_REG_COMMAND          0x12
#define ENS160_REG_TEMP_IN          0x13
#define ENS160_REG_RH_IN            0x15
#define ENS160_REG_TVOC             0x22
#define ENS160_REG_ECO2             0x24
#define ENS160_REG_DATA_STATUS      0x20
#define ENS160_REG_DEVICE_STATUS    0x20

// AHT21-M Registers
#define AHT21_REG_STATUS            0x00
#define AHT21_REG_INIT              0xBE
#define AHT21_REG_MEASURE           0xAC
#define AHT21_REG_RESET             0xBA

// ENS160 Operating Modes
#define ENS160_OPMODE_RESET         0xF0
#define ENS160_OPMODE_IDLE          0x00
#define ENS160_OPMODE_STANDARD      0x02

// AHT21-M Commands
#define AHT21_CMD_INIT              0xBE
#define AHT21_CMD_MEASURE           0xAC
#define AHT21_CMD_RESET             0xBA

typedef struct {
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t ens160_dev_handle;
    i2c_master_dev_handle_t aht21_dev_handle;
} ens160_aht21_handle_t;

// Function declarations
esp_err_t ens160_aht21_init(ens160_aht21_handle_t *handle, i2c_master_bus_handle_t bus_handle);
esp_err_t ens160_read_data(ens160_aht21_handle_t *handle, uint16_t *tvoc, uint16_t *eco2);
esp_err_t aht21_read_data(ens160_aht21_handle_t *handle, float *temperature, float *humidity);
esp_err_t ens160_aht21_read_all_data(ens160_aht21_handle_t *handle, float *temperature, float *humidity, uint16_t *tvoc, uint16_t *eco2);

#endif // ENS160_AHT21_H