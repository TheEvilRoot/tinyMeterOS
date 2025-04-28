#ifndef NFCSWITCHOS_HAL_PIN_HH
#define NFCSWITCHOS_HAL_PIN_HH

#include <freertos/FreeRTOS.h>

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


#endif //NFCSWITCHOS_HAL_PIN_HH
