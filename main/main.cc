#include <array>
#include <cstdio>
#include <cmath>
#include <utility>
#include <sys/unistd.h>

#include "hal_display.hh"
#include "hal_ina_219.hh"
#include "hal_pin.hh"
#include "sliding_buffer.hh"
#include "point.hh"
#include "meter_bus.hh"
#include "worker_task.hh"
#include "hal_button.hh"

struct DisplayUi {
    struct Theme {
        static constexpr auto FontSmall = &lv_font_unscii_8;
        static constexpr auto FontLarge = &lv_font_unscii_16;

        lv_style_t displayStyle{};
        lv_style_t meterStyle{};
        lv_style_t activeMeterStyle{};
        lv_style_t voltageStyle{};
        lv_style_t currentStyle{};
        lv_style_t powerStyle{};

        Theme() {
            lv_style_init(&displayStyle);
            lv_style_set_bg_color(&displayStyle, lv_color_make(0, 0, 0));
            lv_style_set_bg_opa(&displayStyle, LV_OPA_COVER);

            lv_style_init(&meterStyle);
            lv_style_set_bg_color(&meterStyle, lv_color_hex3(0xFFFFFF));
            lv_style_set_text_color(&meterStyle, lv_color_hex3(0x000000));
            lv_style_set_bg_opa(&meterStyle, LV_OPA_COVER);
            lv_style_set_pad_all(&meterStyle, 0);
            lv_style_set_radius(&meterStyle, 4);

            lv_style_init(&activeMeterStyle);
            lv_style_set_bg_color(&activeMeterStyle, lv_color_make(48, 48, 48));
            lv_style_set_text_color(&activeMeterStyle, lv_color_make(255, 255, 255));

            lv_style_init(&voltageStyle);
            lv_style_set_text_align(&voltageStyle, LV_TEXT_ALIGN_LEFT);
            lv_style_set_text_font(&voltageStyle, FontLarge);

            lv_style_init(&currentStyle);
            lv_style_set_text_font(&currentStyle, FontSmall);
            lv_style_set_text_align(&currentStyle, LV_TEXT_ALIGN_RIGHT);

            lv_style_init(&powerStyle);
            lv_style_set_text_font(&powerStyle, FontSmall);
            lv_style_set_text_align(&powerStyle, LV_TEXT_ALIGN_RIGHT);
        }
    };

    struct MeterUi {
        constexpr static lv_coord_t CanvasWidth = 225;
        constexpr static lv_coord_t CanvasHeight = 50;
        static constexpr lv_coord_t MeterHeight = 80;

        lv_obj_t *parent;
        Theme& theme;
        int index;
        SlidingBuffer buffer{CanvasWidth};

        lv_obj_t *layout{};
        lv_obj_t *voltage_label{};
        lv_obj_t *current_label{};
        lv_obj_t *power_label{};
        lv_obj_t* canvas{};
        lv_obj_t* on_led{};


        MeterUi(lv_obj_t* parent, Theme& theme, const int index) : parent{parent}, theme{theme}, index{index}{
            layout = CreateLayout();
            voltage_label = CreateVoltageLabel();
            current_label = CreateCurrentLabel();
            power_label = CreatePowerLabel();
            canvas = CreateCanvas();
            on_led = CreateLed();
        }

        [[nodiscard]] lv_obj_t *CreateLayout() const {
            lv_obj_t *layout = lv_obj_create(parent);
            lv_obj_set_size(layout, lv_obj_get_width(parent), MeterHeight);
            lv_obj_set_pos(layout, 0, index * (MeterHeight + 2));
            lv_obj_add_style(layout, &theme.meterStyle, LV_PART_MAIN);
            lv_obj_add_style(layout, &theme.activeMeterStyle, LV_STATE_USER_1);
            lv_obj_set_scrollbar_mode(layout, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_clip_corner(layout, true, 0);
            return layout;
        }

        [[nodiscard]] lv_obj_t *CreateLed() const {
            auto* led = lv_led_create(layout);
            lv_obj_set_size(led, 7, 7);
            lv_led_set_brightness(led, LV_LED_BRIGHT_MAX);
            lv_led_set_color(led, lv_color_make(255, 0, 0));
            lv_obj_align(led, LV_ALIGN_TOP_LEFT, index == 0 ? 15 : 10, 10);
            return led;
        }

        [[nodiscard]] lv_obj_t *CreateCanvas() const {
            auto *buf = lv_mem_alloc(2 * CanvasWidth * CanvasHeight);
            lv_obj_t *canvas = lv_canvas_create(layout);
            lv_canvas_set_buffer(canvas, buf, CanvasWidth, CanvasHeight, LV_IMG_CF_TRUE_COLOR);
            lv_canvas_fill_bg(canvas, lv_color_hex3(0x3a3a3a), LV_OPA_COVER);
            lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 5, 25);
            return canvas;
        }

        [[nodiscard]] lv_obj_t *CreateVoltageLabel() const {
            lv_obj_t *label = lv_label_create(layout);
            lv_obj_set_width(label, 100);
            lv_obj_align(label, LV_ALIGN_TOP_LEFT, index == 0 ? 30 : 25, 5);
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
            lv_obj_add_style(label, &theme.voltageStyle, 0);
            return label;
        }

        [[nodiscard]] lv_obj_t *CreateCurrentLabel() const {
            lv_obj_t *label = lv_label_create(layout);
            lv_obj_set_width(label, 100);
            lv_obj_align(label, LV_ALIGN_TOP_RIGHT, index == 0 ? -20 : -10, 2);
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
            lv_obj_add_style(label, &theme.currentStyle, 0);
            return label;
        }

        [[nodiscard]] lv_obj_t *CreatePowerLabel() const {
            lv_obj_t *label = lv_label_create(layout);
            lv_obj_set_width(label, 100);
            lv_obj_align(label, LV_ALIGN_TOP_RIGHT, index == 0 ? -10 : -10, 15);
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
            lv_obj_add_style(label, &theme.powerStyle, 0);
            return label;
        }
    };

    hal::Display display{};
    Theme theme{};
    MeterUi dc_ui{display.screen, theme, 0};
    MeterUi usb2_ui{display.screen, theme, 1};
    MeterUi usb1_ui{display.screen, theme, 2};

    int selected{0};

    DisplayUi() {
        lv_obj_add_style(display.screen, &theme.displayStyle, LV_PART_MAIN);
        display.Backlight(true);
    }

    MeterUi* GetUi(const int index) {
        switch (index) {
            case 0: return &dc_ui;
            case 1: return &usb2_ui;
            case 2: return &usb1_ui;
            default: return nullptr;
        }
    }

    void UpdateMeter(int index, const MeterBus& meter) {
        char buffer[48]{};
        if (MeterUi* ui_ptr = GetUi(index); ui_ptr != nullptr) {
            MeterUi& ui = *ui_ptr;

            snprintf(buffer, sizeof(buffer), "%.2fV", meter.voltage);
            lv_label_set_text(ui.voltage_label, buffer);
            if (meter.current > 1.5) {
                snprintf(buffer, sizeof(buffer), "%.3fA", meter.current);
            } else {
                snprintf(buffer, sizeof(buffer), "%dmA", static_cast<int>(meter.current * 1000));
            }
            lv_label_set_text(ui.current_label, buffer);
            if (meter.power > 1) {
                snprintf(buffer, sizeof(buffer), "%.2fW", meter.power);
            } else {
                snprintf(buffer, sizeof(buffer), "%dmW", static_cast<int>(meter.power * 1000));
            }
            lv_label_set_text(ui.power_label, buffer);
            if (meter.enabled) {
                lv_led_on(ui.on_led);
            } else {
                lv_led_off(ui.on_led);
            }

            if (selected == index && !lv_obj_has_state(ui.layout, LV_STATE_USER_1)) {
                lv_obj_add_state(ui.layout, LV_STATE_USER_1);
            } else if (selected != index && lv_obj_has_state(ui.layout, LV_STATE_USER_1)) {
                lv_obj_clear_state(ui.layout, LV_STATE_USER_1);
            }

            if (meter.enabled) {
                ui.buffer.Push(meter.GetPoint());
            }

            lv_canvas_fill_bg(ui.canvas, lv_color_hex3(0x000000), LV_OPA_COVER);
            for (auto i = ui.buffer.start; i < ui.buffer.start + ui.buffer.window; i++) {
                const auto& point = ui.buffer.values[i];
                auto y = MeterUi::CanvasHeight - 1 - point.VoltageY(MeterUi::CanvasHeight);
                if (y <= 0 || y >= MeterUi::CanvasHeight) {
                    ESP_LOGE("DisplayUi", "Bad voltage Y:%d V:%u", y, point.voltage);
                    continue;
                }
                lv_canvas_set_px(ui.canvas, i - ui.buffer.start, y, lv_color_make(255, 255, 0));
                y = MeterUi::CanvasHeight - 1 - point.CurrentY(MeterUi::CanvasHeight);
                if (y <= 0 || y >= MeterUi::CanvasHeight) {
                    ESP_LOGE("DisplayUi", "Bad current Y:%d I:%d", y, point.current);
                    continue;
                }
                lv_canvas_set_px(ui.canvas, i - ui.buffer.start, MeterUi::CanvasHeight - 1 - point.CurrentY(MeterUi::CanvasHeight),
                    lv_color_make(0, 255, 255));
            }
        }
    }
};

struct Meter {
    hal::I2CBus i2cBus{34, 35};
    std::array<MeterBus, 3> buses{
        MeterBus{i2cBus, 0x41, 37, "DC Jack"},
        MeterBus{i2cBus, 0x44, 36, "USB-2"},
        MeterBus{i2cBus, 0x40, 33, "USB-1"},
    };
    DisplayUi display{};
};


struct Keypad {
    Meter& meter;
    Queue<hal::Keypress> queue{16};
    hal::Button up{11, queue};
    hal::Button right{10, queue};
    hal::Button center{9, queue};
    hal::Button down{8, queue};
    hal::Button left{7, queue};

    explicit Keypad(Meter& meter) : meter{meter} {
        WorkerTask<Keypad>([](Keypad& keypad) {
            while (true) {
                hal::Keypress keyPress{};
                if (keypad.queue.Receive(keyPress)) {
                    keypad.HandleKeypress(keyPress);
                }
            }
        }, *this);
    }

    hal::Button* GetButton(int gpio) {
        if (up.pin.gpio == gpio) {
            return &up;
        }
        if (right.pin.gpio == gpio) {
            return &right;
        }
        if (center.pin.gpio == gpio) {
            return &center;
        }
        if (down.pin.gpio == gpio) {
            return &down;
        }
        if (left.pin.gpio == gpio) {
            return &left;
        }
        return nullptr;
    }

    void HandleKeypress(const hal::Keypress& keyPress) {
        auto button = GetButton(keyPress.gpio);
        if (button == nullptr) { return; }
        switch (keyPress.event) {
            case hal::Keypress::Event::kShortPress:
                OnPress(button);
                break;
            case hal::Keypress::Event::kLongPress:
                OnLongPress(button);
                break;
            case hal::Keypress::Event::kLongPressUp:
                OnLongRelease(button);
                break;
            default:
                break;
        }
    }

    void OnPress(hal::Button* button) {
        if (button == &up) {
            if (meter.display.selected == 0) {
                meter.display.selected = 2;
            } else {
                meter.display.selected--;
            }
        } else if (button == &down) {
            if (meter.display.selected == 2) {
                meter.display.selected = 0;
            } else {
                meter.display.selected++;
            }
        } else if (button == &center) {
            if (meter.buses[meter.display.selected].enabled) {
                meter.buses[meter.display.selected].Disable();
            } else {
                meter.buses[meter.display.selected].Enable();
            }
        }
    }

    void OnLongPress(hal::Button*) {

    }

    void OnLongRelease(hal::Button*) {
    }
};

extern "C" void app_main(void) {
    Meter meter{};
    Keypad keypad{meter};

    lv_timer_create([](lv_timer_t* timer) {
        Meter& meter = *static_cast<Meter*>(timer->user_data);
        for (auto i = 0; i < meter.buses.size(); i++) {
            meter.display.UpdateMeter(i, meter.buses[i]);
        }
    }, 20, &meter);

    auto statsTimer = xTimerCreate("Stats", pdMS_TO_TICKS(5000), true, nullptr, [](TimerHandle_t) {
        auto freeHeap = esp_get_free_heap_size();
        ESP_LOGI("Stats", "heap free:%.2fkB", static_cast<float>(freeHeap) / 1024.);
    });
    xTimerStart(statsTimer, 0);

    uint64_t sent = 0;
    uint64_t ts = 0;
    float thr = 0;
    while (true) {
        Packet bulk[3]{};
        for (uint8_t i = 0; i < meter.buses.size(); i++) {
            meter.buses[i].Update();
            bulk[i] = meter.buses[i].GetPacket();
        }
        sent += sizeof(bulk);
        constexpr int kSeconds = 10;
        constexpr float kAlpha = 0.3f;
        if (ts == 0 || xTaskGetTickCount() - ts > 1000 * kSeconds) {
            ts = xTaskGetTickCount();
            thr = (1.0f - kAlpha) * thr + kAlpha * static_cast<float>(sent);
            ESP_LOGI("Meter", "Sent %llu bytes/s throughput %.1f bytes/s", sent / kSeconds, thr / kSeconds);
            sent = 0;
        }
        usleep(50000);
    }
}
