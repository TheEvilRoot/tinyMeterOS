#include <array>
#include <cstdio>
#include <cmath>
#include <sys/unistd.h>

#include "hal_display.hh"
#include "hal_ina_219.hh"
#include "hal_pin.hh"
#include "sliding_buffer.hh"
#include "point.hh"
#include "meter_bus.hh"

struct DisplayUi {
    constexpr static size_t CanvasWidth = 220;
    constexpr static size_t CanvasHeight = 50;

    hal::Display display{};

    struct MeterUi {

        static constexpr auto FontSmall = &lv_font_unscii_8;
        static constexpr auto FontLarge = &lv_font_unscii_16;

        SlidingBuffer buffer{CanvasWidth};
        lv_obj_t *voltage_label{};
        lv_obj_t *current_label{};
        lv_obj_t *power_label{};
        lv_obj_t* canvas{};
        lv_obj_t* on_led{};
        lv_obj_t* select_ind{};

        MeterUi(const hal::Display& display, const int index) {
            voltage_label = CreateVoltageLabel(display, index);
            current_label = CreateCurrentLabel(display, index);
            power_label = CreatePowerLabel(display, index);
            canvas = CreateCanvas(display, index);
            on_led = CreateLed(display, index);
            select_ind = CreateLine(display, index);
        }

        [[nodiscard]] static lv_obj_t *CreateLine(const hal::Display& display, const int index) {
            static lv_style_t style = []() {
                lv_style_t st{};
                lv_style_init(&st);
                lv_style_set_line_width(&st, 5);
                lv_style_set_line_color(&st, lv_color_hex3(0xFFF));
                lv_style_set_line_rounded(&st, true);
                return st;
            }();
            auto* line = lv_line_create(display.screen);
            auto *points = new lv_point_t[2]{
                {10, static_cast<int16_t>(80 * index + 25)},
                {10, static_cast<int16_t>(80 * index + 90)},
            };
            lv_line_set_points(line, points, 2);
            lv_obj_add_style(line, &style, 0);
            return line;
        }

        [[nodiscard]] static lv_obj_t *CreateLed(const hal::Display& display, const int index) {
            auto* led = lv_led_create(display.screen);
            lv_obj_set_size(led, 10, 10);
            lv_led_set_brightness(led, 150);
            lv_led_set_color(led, lv_color_hex3(0x0F0));
            lv_obj_align(led, LV_ALIGN_TOP_LEFT, 15, 80 * index + 25);
            return led;
        }

        [[nodiscard]] static lv_obj_t *CreateCanvas(const hal::Display& display, const int index) {
            auto *buf = lv_mem_alloc(2 * CanvasWidth * CanvasHeight);
            lv_obj_t *canvas = lv_canvas_create(display.screen);
            lv_canvas_set_buffer(canvas, buf, CanvasWidth, CanvasHeight, LV_IMG_CF_TRUE_COLOR);
            lv_canvas_fill_bg(canvas, lv_color_hex3(0xFFF), LV_OPA_COVER);
            lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 15, 80 * index + 40);
            return canvas;
        }

        [[nodiscard]] static lv_obj_t *CreateVoltageLabel(const hal::Display& display, const int index) {
            static lv_style_t style = []() {
                lv_style_t st{};
                lv_style_init(&st);
                lv_style_set_text_align(&st, LV_TEXT_ALIGN_LEFT);
                lv_style_set_text_font(&st, FontLarge);
                return st;
            }();
            lv_obj_t *label = lv_label_create(display.screen);
            lv_obj_set_width(label, 100);
            lv_obj_align(label, LV_ALIGN_TOP_LEFT, 30, 80 * index + 20);
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
            lv_obj_add_style(label, &style, 0);
            return label;
        }

        [[nodiscard]] static lv_obj_t *CreateCurrentLabel(const hal::Display& display, const int index) {
            static lv_style_t style = []() {
                lv_style_t st{};
                lv_style_init(&st);
                lv_style_set_text_font(&st, FontSmall);
                lv_style_set_text_align(&st, LV_TEXT_ALIGN_RIGHT);
                return st;
            }();
            lv_obj_t *label = lv_label_create(display.screen);
            lv_obj_set_width(label, 100);
            lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -10, 80 * index + 15);
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
            lv_obj_add_style(label, &style, 0);
            return label;
        }

        [[nodiscard]] static lv_obj_t *CreatePowerLabel(const hal::Display& display, const int index) {
            static lv_style_t style = []() {
                lv_style_t st{};
                lv_style_init(&st);
                lv_style_set_text_font(&st, FontSmall);
                lv_style_set_text_align(&st, LV_TEXT_ALIGN_RIGHT);
                return st;
            }();
            lv_obj_t *label = lv_label_create(display.screen);
            lv_obj_set_width(label, 100);
            lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -10, 80 * index + 15 + 15);
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
            lv_obj_add_style(label, &style, 0);
            return label;
        }
    };

    MeterUi dc_ui{display, 0};
    MeterUi usb2_ui{display, 1};
    MeterUi usb1_ui{display, 2};

    int selected{0};

    DisplayUi() {
        // 0x000f - yellow
        // 0x001e - yellow
        // 0x006f - orange
        // 0x00bf - very orange
        // 0x0110 - white
        // 0x0f3f - nice green
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
            lv_led_set_color(ui.on_led, lv_color_hex3(meter.enabled ? 0x0F3F : 0x00BF));


            if (selected == index) {
                lv_obj_clear_flag(ui.select_ind, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(ui.select_ind, LV_OBJ_FLAG_HIDDEN);
            }

            if (meter.enabled) {
                ui.buffer.Push(meter.GetPoint());
            }
            lv_canvas_fill_bg(ui.canvas, lv_color_hex3(0xFFF), LV_OPA_COVER);
            for (auto i = ui.buffer.start; i < ui.buffer.start + ui.buffer.window; i++) {
                const auto& point = ui.buffer.values[i];
                auto y = CanvasHeight - 1 - point.VoltageY(CanvasHeight);
                if (y <= 0 || y >= CanvasHeight) {
                    ESP_LOGE("DisplayUi", "Bad voltage Y:%d V:%u", y, point.voltage);
                    continue;
                }
                lv_canvas_set_px(ui.canvas, i - ui.buffer.start, y, lv_color_hex3(0xF00));
                y = CanvasHeight - 1 - point.CurrentY(CanvasHeight);
                if (y <= 0 || y >= CanvasHeight) {
                    ESP_LOGE("DisplayUi", "Bad current Y:%d I:%d", y, point.current);
                    continue;
                }
                lv_canvas_set_px(ui.canvas, i - ui.buffer.start, CanvasHeight - 1 - point.CurrentY(CanvasHeight), lv_color_hex3(0x0F0));
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
    hal::Pin up{11, hal::PinMode::kInput, hal::PinState::kPullDown};
    hal::Pin right{10, hal::PinMode::kInput, hal::PinState::kPullDown};
    hal::Pin center{9, hal::PinMode::kInput, hal::PinState::kPullDown};
    hal::Pin down{8, hal::PinMode::kInput, hal::PinState::kPullDown};
    hal::Pin left{7, hal::PinMode::kInput, hal::PinState::kPullDown};

    Keypad(Meter& meter) {
        down.AttachInterrupt([](void* arg) {
            Meter& meter = *static_cast<Meter*>(arg);
            auto selected = meter.display.selected + 1;
            if (selected >= 3) {
                selected = 0;
            }
            meter.display.selected = selected;
        }, &meter, hal::IntrEdge::kRisingEdge);
        up.AttachInterrupt([](void* arg) {
            Meter& meter = *static_cast<Meter*>(arg);
            auto selected = meter.display.selected - 1;
            if (selected < 0) {
                selected = 2;
            }
            meter.display.selected = selected;
        }, &meter, hal::IntrEdge::kRisingEdge);
        center.AttachInterrupt([](void* arg) {
            Meter& meter = *static_cast<Meter*>(arg);
            auto selected = meter.display.selected;
            auto enabled = meter.buses[selected].enabled;
            if (enabled) {
                meter.buses[selected].Disable();
            } else {
                meter.buses[selected].Enable();
            }
        }, &meter, hal::IntrEdge::kRisingEdge);
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

    while (true) {
        for (auto& bus : meter.buses) { bus.Update(); }
        usleep(50000);
    }
}
