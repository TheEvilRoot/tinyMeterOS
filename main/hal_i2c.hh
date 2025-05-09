#ifndef HAL_I2C_HH
#define HAL_I2C_HH

#include <driver/i2c.h>
#include <driver/i2c_master.h>
#include <esp_log.h>

namespace hal {
    struct I2CDevice {
        i2c_master_dev_handle_t device{};

        I2CDevice(const i2c_master_bus_handle_t &bus, const uint16_t address);

        ~I2CDevice();

        [[nodiscard]] esp_err_t Transmit(const uint8_t* tx, size_t txSize, uint8_t* rx, size_t rxSize) const;

        [[nodiscard]] esp_err_t Send(const uint8_t* tx, size_t txSize) const;
    };

    struct I2CBus {
        i2c_master_bus_handle_t bus{};

        I2CBus(int sda, int scl);

        ~I2CBus();

        [[nodiscard]] I2CDevice NewDevice(uint16_t address) const;
    };
}



#endif // HAL_I2C_HH
