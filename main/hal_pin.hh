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

  struct Button {
    constexpr static int kPressThresholdMs = 50;
    constexpr static int kDebounceMs = 10;

    Pin pin;
    int downTicks{0};

    void* pressDownArg{nullptr};
    void* pressUpArg{nullptr};
    void* longPressDownArg{nullptr};
    void* longPressUpArg{nullptr};

    void (*pressDownHandler)(void*){nullptr};
    void (*pressUpHandler)(void*){nullptr};
    void (*longPressDownHandler)(void*){nullptr};
    void (*longPressUpHandler)(void*){nullptr};

    explicit Button(int gpio, PinState state = PinState::kPullDown) :
        pin{gpio, PinMode::kInput, state} {
      pin.AttachInterrupt([](void* arg) {
        auto &self = *static_cast<Button*>(arg);
        self.OnDown();
      }, this, IntrEdge::kRisingEdge);
      pin.AttachInterrupt([](void* arg) {
        auto &self = *static_cast<Button*>(arg);
        self.OnUp();
      }, this, IntrEdge::kFallingEdge);
    }

    void OnUp() {
    }

    void OnDown() {
      downTicks = xTaskGetTickCount();
    }

    // Short press handler, called when button released before kPressThresholdMs and after kDebounceMs
    void OnPressUp(void(*handler)(void*), void* arg){
      pressUpArg = arg;
      pressUpHandler = handler;
    }

    // Long press handler, called after kPressThresholdMs since button pressed and not yet released
    void OnLongPressDown(void(*handler)(void*), void* arg) {
      longPressDownHandler = handler;
      longPressDownArg = arg;
    }

    // Long press handler, called when button released after at least kPressThresholdMs
    void OnLongPressUp(void(*handler)(void*), void* arg) {
      longPressUpArg = arg;
      longPressUpHandler = handler;
    }
  };
}


#endif //NFCSWITCHOS_HAL_PIN_HH
