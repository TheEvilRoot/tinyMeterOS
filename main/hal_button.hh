#ifndef HAL_BUTTON_HH
#define HAL_BUTTON_HH

#include <freertos/FreeRTOS.h>

#include "hal_pin.hh"
#include "queue.hh"

namespace hal {
  struct Keypress {
    int gpio;
    TickType_t dt;
    enum class Event {
      kShortPress = 1,
      kLongPress = 2,
      kLongPressUp = 3,
    } event;
  };

  struct Button {
    Pin pin;
    Queue<Keypress> queue;

    bool down{false};
    bool longPress{false};
    bool timerStarted{false};
    bool debounced{true};
    TickType_t pressTime{0};
    TimerHandle_t longPressTimer{};
    TimerHandle_t debounceTimer{};

    explicit Button(int gpio, const Queue<Keypress>& queue);

    ~Button();

    void Debounce();

    void StartLongPressTimer();

    void StopLongPressTimer();

    [[nodiscard]] bool IsPressed() const;

    void OnShortPress(TickType_t dt);

    void OnLongPress(TickType_t dt);

    void OnLongPressUp(TickType_t dt);
  };
}


#endif //HAL_BUTTON_HH
