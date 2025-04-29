#include "meter_bus.hh"

MeterBus::MeterBus(const hal::I2CBus &bus, uint16_t address, int gpio, const char *name):
    name{name},
    ina{bus, 100.0, address},
    nfet{gpio, hal::PinMode::kOutput, hal::PinState::kFloat} {
    Reset();
}

void MeterBus::Reset() {
    enabled = false;
    current = 0;
    voltage = 0;
    power = 0;
    nfet.SetState(false);
    ina.Reset();
    ina.Calibrate();
}

void MeterBus::Enable() {
    enabled = true;
    nfet.SetState(true);
}

void MeterBus::Disable() {
    enabled = false;
    nfet.SetState(false);
}

void MeterBus::Update() {
    voltage = ina.ReadBusVoltage();
    current = ina.ReadCurrent();
    power = ina.ReadPower();
}

Point MeterBus::GetPoint() const {
    return {voltage,current,power};
}
