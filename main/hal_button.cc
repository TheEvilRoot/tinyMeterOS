#include "hal_button.hh"

hal::Button::Button(int gpio, const Queue<Keypress> &queue): pin{gpio, PinMode::kInput, PinState::kFloat}, queue{queue} {
    pin.AttachInterrupt([](void* arg) {
        auto& button = *static_cast<Button*>(arg);
        if (!button.debounced) { return; }
        if (button.IsPressed()) {
            if (!button.down && !button.timerStarted) {
                button.down = true;
                button.pressTime = xTaskGetTickCountFromISR();
                button.StartLongPressTimer();
            }
        } else if (button.down) {
            button.down = false;
            button.StopLongPressTimer();
            if (button.longPress) {
                button.OnLongPressUp(xTaskGetTickCountFromISR() - button.pressTime);
            } else {
                button.OnShortPress(xTaskGetTickCountFromISR() - button.pressTime);
            }
            button.longPress = false;
            button.Debounce();
        }
    }, this, IntrEdge::kBothEdge);
    longPressTimer = xTimerCreate("Button", pdMS_TO_TICKS(1500), false, this, [](TimerHandle_t timer) {
        auto& button = *static_cast<Button*>(pvTimerGetTimerID(timer));
        if (!button.down || !button.timerStarted) {
            return;
        }
        button.longPress = true;
        button.timerStarted = false;
        button.OnLongPress(xTaskGetTickCountFromISR() - button.pressTime);
    });
    debounceTimer = xTimerCreate("Button", pdMS_TO_TICKS(50), false, this, [](TimerHandle_t timer) {
        auto& button = *static_cast<Button*>(pvTimerGetTimerID(timer));
        button.debounced = true;
    });
}

hal::Button::~Button() {
    StopLongPressTimer();
    xTimerDelete(longPressTimer, 0);
}

void hal::Button::Debounce() {
    debounced = false;
    xTimerStart(debounceTimer, 0);
}

void hal::Button::StartLongPressTimer() {
    timerStarted = true;
    xTimerStartFromISR(longPressTimer, 0);
}

void hal::Button::StopLongPressTimer() {
    timerStarted = false;
    xTimerStopFromISR(longPressTimer, 0);
}

bool hal::Button::IsPressed() const { return pin.GetState(); }

void hal::Button::OnShortPress(TickType_t dt) {
    if (dt < 50) { return; }
    queue.PushFromIsr({pin.gpio, dt, Keypress::Event::kShortPress});
}

void hal::Button::OnLongPress(TickType_t dt) {
    if (dt < 50) { return; }
    queue.PushFromIsr({pin.gpio, dt, Keypress::Event::kLongPress});
}

void hal::Button::OnLongPressUp(TickType_t dt) {
    if (dt < 50) { return; }
    queue.PushFromIsr({pin.gpio, dt, Keypress::Event::kLongPressUp});
}
