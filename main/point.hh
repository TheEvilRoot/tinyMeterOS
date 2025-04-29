#ifndef POINT_HH
#define POINT_HH

#include <cstdlib>

struct Point {
    static constexpr float MaxVoltage = 26000.0f;
    static constexpr float MaxCurrent = 3000.0f;

    uint16_t voltage;
    int16_t current;
    uint16_t power;

    Point() :
        voltage{0},
        current{0},
        power{0} { }

    Point(float voltage, float current, float power);

    [[nodiscard]] int VoltageY(size_t height) const;

    [[nodiscard]] int CurrentY(size_t height) const;

    [[nodiscard]] int PowerY(size_t height) const;
};



#endif //POINT_HH
