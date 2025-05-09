#ifndef HAL_INA_219_H
#define HAL_INA_219_H

#include "hal_i2c.hh"

namespace hal {
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

    Ina219(const I2CBus& bus, float shunt, uint16_t address);

    [[nodiscard]] uint16_t ReadRegister(uint8_t reg) const;

    void WriteRegister(uint8_t reg, uint16_t value) const;

    [[nodiscard]] float ReadShuntVoltage() const;

    [[nodiscard]] float ReadBusVoltage() const;

    [[nodiscard]] float ReadPower() const;

    [[nodiscard]] float ReadCurrent() const;

    void Reset() const;

    void ShutDown() const;

    void Configure(uint16_t config) const;

    void Calibrate() const;
  };
}


#endif // HAL_INA_219_H
