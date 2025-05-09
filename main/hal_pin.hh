#ifndef HAL_PIN_H
#define HAL_PIN_H

namespace hal {
  enum class PinMode {
    kInput,
    kOutput,
  };

  enum class PinState {
    kFloat,
    kPullUp,
    kPullDown,
  };

  enum class IntrEdge {
    kRisingEdge,
    kFallingEdge,
    kBothEdge,
  };

  struct Pin {
    int gpio;
    PinMode mode;
    PinState state;

    explicit Pin(int gpio, PinMode mode, PinState state = PinState::kFloat);

    [[nodiscard]] bool GetState() const;

    void SetState(bool state) const;

    void AttachInterrupt(void(*handler)(void*), void* arg, IntrEdge edge) const;
  };
}


#endif // HAL_PIN_H
