#ifndef METER_BUS_HH
#define METER_BUS_HH

#include "hal_pin.hh"
#include "hal_ina_219.hh"
#include "point.hh"

struct Packet {
    int32_t voltage{0};
    int32_t current{0};
    uint32_t power{0};
};

struct MeterBus {
    const char* name;
    hal::Ina219 ina;
    hal::Pin nfet;

    bool enabled{false};
    float current{0};
    float voltage{0};
    float power{0};

    MeterBus(const hal::I2CBus& bus, uint16_t address, int gpio, const char* name);

    void Reset();

    void Enable();

    void Disable();

    void Update();

    [[nodiscard]] Point GetPoint() const;
    [[nodiscard]] Packet GetPacket() const;
};

#endif //METER_BUS_HH
