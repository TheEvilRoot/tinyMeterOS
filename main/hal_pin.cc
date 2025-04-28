#include "hal_pin.hh"

#include <driver/gpio.h>

hal::Pin::Pin(int gpio, PinMode mode, PinState state): gpio(gpio), mode{mode}, state{state} {
  gpio_set_direction(static_cast<gpio_num_t>(gpio), mode == PinMode::kInput ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT);
  if (state == PinState::kPullUp) {
    gpio_pullup_en(static_cast<gpio_num_t>(gpio));
    gpio_pulldown_dis(static_cast<gpio_num_t>(gpio));
  } else if (state == PinState::kPullDown) {
    gpio_pulldown_en(static_cast<gpio_num_t>(gpio));
    gpio_pullup_dis(static_cast<gpio_num_t>(gpio));
  } else {
    gpio_pullup_dis(static_cast<gpio_num_t>(gpio));
    gpio_pulldown_dis(static_cast<gpio_num_t>(gpio));
  }
}

bool hal::Pin::GetState() const {
  if (mode != PinMode::kInput) {
    return false;
  }
  return gpio_get_level(static_cast<gpio_num_t>(gpio));
}

void hal::Pin::SetState(bool state) const {
  if (mode != PinMode::kOutput) {
    return;
  }
  gpio_set_level(static_cast<gpio_num_t>(gpio), state);
}

void hal::Pin::AttachInterrupt(void(*handler)(void *), void *arg, IntrEdge edge) const {
  static bool isrServiceInitialized = false;
  if (!isrServiceInitialized) {
    isrServiceInitialized = true;
    gpio_install_isr_service(0);
  }
  if (edge == IntrEdge::kFallingEdge) {
    gpio_set_intr_type(static_cast<gpio_num_t>(gpio), GPIO_INTR_NEGEDGE);
  } else if (edge == IntrEdge::kRisingEdge) {
    gpio_set_intr_type(static_cast<gpio_num_t>(gpio), GPIO_INTR_POSEDGE);
  } else if (edge == IntrEdge::kBothEdge) {
    gpio_set_intr_type(static_cast<gpio_num_t>(gpio), GPIO_INTR_ANYEDGE);
  }
  gpio_isr_handler_add(static_cast<gpio_num_t>(gpio), handler, arg);
}