#include "hal_ina_219.hh"

hal::Ina219::Ina219(const I2CBus &bus, float shunt, uint16_t address): device{bus.NewDevice(address)} {
    currentLsb = MAX_CURRENT * 3.0517578125e-5; // see datasheet please
    const float milliShunt = shunt / 1000.f; // convert Ohm to mOhm
    calibration = static_cast<uint16_t>(CURRENT_SCALE / (currentLsb * milliShunt));
    ESP_LOGI("Ina219", "Initialized I2C for INA219 addr:%02x shunt:%.1fmOhm\n", address, milliShunt);
}

uint16_t hal::Ina219::ReadRegister(uint8_t reg) const {
    uint8_t out[2]{};
    ESP_ERROR_CHECK(device.Transmit(&reg, 1, out, 2));
    return out[0] << 8 | out[1];
}

void hal::Ina219::WriteRegister(uint8_t reg, uint16_t value) const {
    uint8_t out[3]{};
    out[0] = reg;
    out[1] = value >> 8;
    out[2] = value & 0xff;
    ESP_ERROR_CHECK(device.Send(out, 3));
}

float hal::Ina219::ReadShuntVoltage() const {
    uint16_t value = ReadRegister(REG_SHUNT_VOLTS);
    return static_cast<float>(value) * 0.01f;
}

float hal::Ina219::ReadBusVoltage() const {
    uint16_t value = ReadRegister(REG_BUS_VOLTS);
    return static_cast<float>(static_cast<uint16_t>((value >> static_cast<uint16_t>(3)) * 4)) * 0.001f;
}

float hal::Ina219::ReadPower() const {
    int16_t value = ReadRegister(REG_POWER);
    return static_cast<float>(value) * (currentLsb * POWER_SCALE);
}

float hal::Ina219::ReadCurrent() const {
    int16_t value = ReadRegister(REG_CURRENT);
    return static_cast<float>(value) * currentLsb;
}

void hal::Ina219::Reset() const {
    Configure(0x399F);
}

void hal::Ina219::ShutDown() const {
    Configure(0x399F & ~0x7); // 3 LSB bits are MODE, 000 for shutdown
}

void hal::Ina219::Configure(uint16_t config) const {
    WriteRegister(REG_CONFIG, config);
}

void hal::Ina219::Calibrate() const {
    WriteRegister(REG_CALIBRATION, calibration);
}
