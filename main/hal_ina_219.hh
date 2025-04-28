
#ifndef NFCSWITCHOS_HAL_INA_219_HH
#define NFCSWITCHOS_HAL_INA_219_HH

#include <driver/i2c.h>
#include <driver/i2c_master.h>

#include "esp_log.h"

namespace hal {
  struct I2CDevice {
    i2c_master_dev_handle_t device{};

    I2CDevice(const i2c_master_bus_handle_t &bus, const uint16_t address) {
      i2c_device_config_t dev_config{};
      dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
      dev_config.device_address = address;
      dev_config.scl_speed_hz = 1000000;
      ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_config, &device));
      ESP_LOGI("I2C", "I2C device initialized address:%04x", address);
    }

    ~I2CDevice() {
      i2c_master_bus_rm_device(device);
    }

    [[nodiscard]] esp_err_t Transmit(const uint8_t* tx, size_t txSize, uint8_t* rx, size_t rxSize) const {
      return i2c_master_transmit_receive(device, tx, txSize, rx, rxSize, 1000);
    }

    [[nodiscard]] esp_err_t Send(const uint8_t* tx, size_t txSize) const {
      return i2c_master_transmit(device, tx, txSize, 1000);
    }
  };

  struct I2CBus {
    i2c_master_bus_handle_t bus{};

    I2CBus(int sda, int scl) {
      i2c_master_bus_config_t bus_config{};
      bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
      bus_config.i2c_port = I2C_NUM_1;
      bus_config.sda_io_num = static_cast<gpio_num_t>(sda);
      bus_config.scl_io_num = static_cast<gpio_num_t>(scl);
      ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));
      ESP_LOGI("I2CBus", "Initialized I2C bus SDA:%d SCL:%d\n", sda, scl);
    }
    ~I2CBus() {
      i2c_del_master_bus(bus);
    }

    [[nodiscard]] I2CDevice NewDevice(const uint16_t address) const {
      return I2CDevice(bus, address);
    }
  };

  struct Ina219 {

    static constexpr float CURRENT_SCALE = 0.04096;
    static constexpr float POWER_SCALE = 20;
    static constexpr float MAX_CURRENT = 5.0; // Max current we're expecting on shunt

    static constexpr uint8_t REG_CONFIG = 0x00;
    static constexpr uint8_t REG_SHUNT_VOLTS = 0x01;
    static constexpr uint8_t REG_BUS_VOLTS = 0x02;
    static constexpr uint8_t REG_POWER = 0x03;
    static constexpr uint8_t REG_CURRENT = 0x04;
    static constexpr uint8_t REG_CALIBRATION = 0x05;

    I2CDevice device;
    float currentLsb{0};
    uint16_t calibration{0};

    Ina219(const I2CBus& bus, float shunt, uint16_t address) : device{bus.NewDevice(address)} {
      currentLsb = MAX_CURRENT * 3.0517578125e-5; // see datasheet please
      const float milliShunt = shunt / 1000.; // convert Ohm to mOhm
      calibration = static_cast<uint16_t>(CURRENT_SCALE / (currentLsb * milliShunt));
      ESP_LOGI("Ina219", "Initialized I2C for INA219 addr:%02x shunt:%.1fmOhm\n", address, milliShunt);
  }

    uint16_t ReadRegister(uint8_t reg) {
      uint8_t out[2]{};
      ESP_ERROR_CHECK(device.Transmit(&reg, 1, out, 2));
      return out[0] << 8 | out[1];
    }

    void WriteRegister(uint8_t reg, uint16_t value) {
      uint8_t out[3]{};
      out[0] = reg;
      out[1] = value >> 8;
      out[2] = value & 0xff;
      ESP_ERROR_CHECK(device.Send(out, 3));
    }

    float ReadShuntVoltage() {
      uint16_t value = ReadRegister(REG_SHUNT_VOLTS);
      return static_cast<float>(value) * 0.01f;
    }

    float ReadBusVoltage() {
      uint16_t value = ReadRegister(REG_BUS_VOLTS);
      return static_cast<float>(static_cast<uint16_t>((value >> static_cast<uint16_t>(3)) * 4)) * 0.001f;
    }

    float ReadPower() {
      int16_t value = ReadRegister(REG_POWER);
      return static_cast<float>(value) * (currentLsb * POWER_SCALE);
    }

    float ReadCurrent() {
      int16_t value = ReadRegister(REG_CURRENT);
      return static_cast<float>(value) * currentLsb;
    }

    void Reset() {
      Configure(0x399F);
    }

    void ShutDown() {
      Configure(0x399F & ~0x7); // 3 LSB bits are MODE, 000 for shutdown
    }

    void Configure(uint16_t config) {
      WriteRegister(REG_CONFIG, config);
    }

    void Calibrate() {
      WriteRegister(REG_CALIBRATION, calibration);
    }
  };
}


#endif //NFCSWITCHOS_HAL_INA_219_HH
