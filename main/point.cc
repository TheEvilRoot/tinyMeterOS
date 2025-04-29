#include "point.hh"

Point::Point(float voltage, float current, float power):
    voltage{static_cast<uint16_t>(voltage * 1000.0)},
    current{static_cast<int16_t>(current * 1000.0)},
    power{static_cast<uint16_t>(power * 1000.0)} { }

int Point::VoltageY(const size_t height) const {
    const auto f = static_cast<float>(voltage) / MaxVoltage; // NOLINT(*-narrowing-conversions)
    return static_cast<int>(f * height);  // NOLINT(*-narrowing-conversions)
}

int Point::CurrentY(const size_t height) const {
    const auto f = static_cast<float>(abs(current)) / MaxCurrent;  // NOLINT(*-narrowing-conversions)
    return static_cast<int>(f * height);  // NOLINT(*-narrowing-conversions)
}

int Point::PowerY(const size_t height) const {
    const auto f = static_cast<float>(power) / (MaxCurrent * MaxVoltage);  // NOLINT(*-narrowing-conversions)
    return static_cast<int>(f * height);  // NOLINT(*-narrowing-conversions)
}
