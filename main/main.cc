#include <array>
#include <stdio.h>
#include <sys/unistd.h>

#include "hal_display.hh"
#include "hal_ina_219.hh"
#include "hal_pin.hh"
#include <math.h>

struct Point {
    static constexpr float MaxVoltage = 16.0f;
    static constexpr float MaxCurrent = 2.0f;

    float voltage;
    float current;
    float power;

    int VoltageY(const size_t height) const {
        return static_cast<int>(voltage * height / MaxVoltage);
    }

    int CurrentY(const size_t height) const {
        return static_cast<int>(fabs(current) * height / MaxCurrent);
    }

    int PowerY(const size_t height) const {
        return static_cast<int>(power * height / (MaxCurrent * MaxVoltage));
    }
};

struct SlidingBuffer {
    size_t window;
    Point *values;
    size_t start{0};
    size_t pos{0};

    explicit SlidingBuffer(size_t window) : window{window}, values{new Point[window * 2]{}} {
    }

    ~SlidingBuffer() {
        delete values;
    }

    void Push(const Point& value) {
        if (pos >= window * 2) {
            pos = window;
            start = 0;
        }
        values[pos++] = value;
        if (start + pos > window) {
            start++;
            values[pos - window - 1] = value;
        }
    }

    void Reset() {
        start = 0;
        pos = 0;
    }
};

struct MeterBus {
    const char* name;
    hal::Ina219 ina;
    hal::Pin nfet;

    bool enabled{false};
    float current{0};
    float voltage{0};
    float power{0};

    MeterBus(const hal::I2CBus& bus, uint16_t address, int gpio, const char* name) :
        name{name},
        ina{bus, 100.0, address},
        nfet{gpio, hal::PinMode::kOutput, hal::PinState::kFloat} {
        Reset();
    }

    void Reset() {
        enabled = false;
        current = 0;
        voltage = 0;
        power = 0;
        nfet.SetState(false);
        ina.Reset();
        ina.Calibrate();
    }

    void Enable() {
        enabled = true;
        nfet.SetState(true);
    }

    void Disable() {
        enabled = false;
        nfet.SetState(false);
    }

    void Update() {
        if (!enabled) {
            return;
        }
        voltage = ina.ReadBusVoltage();
        current = ina.ReadCurrent();
        power = ina.ReadPower();
    }

    Point GetPoint() const {
        return Point{
            .voltage = voltage,
            .current = current,
            .power = power
        };
    }
};

struct DisplayUi {
    constexpr static size_t CanvasWidth = 220;
    constexpr static size_t CanvasHeight = 50;

    hal::Display display{};

    lv_obj_t *usb1_voltage{};
    lv_obj_t *usb2_voltage{};
    lv_obj_t* dc_voltage{};

    lv_obj_t* usb1_canvas{};
    lv_obj_t* usb2_canvas{};
    lv_obj_t* dc_canvas{};

    SlidingBuffer usb1_buffer{CanvasWidth};
    SlidingBuffer usb2_buffer{CanvasWidth};
    SlidingBuffer dc_buffer{CanvasWidth};

    DisplayUi() {
        display.Begin();

        usb1_voltage = CreateVoltageLabel(0);
        usb2_voltage = CreateVoltageLabel(1);
        dc_voltage = CreateVoltageLabel(2);

        usb1_canvas = CreateCanvas(0);
        usb2_canvas = CreateCanvas(1);
        dc_canvas = CreateCanvas(2);

        display.Backlight(true);
    }

    lv_obj_t *CreateCanvas(int index) const {
        auto *buf = lv_mem_alloc(2 * CanvasWidth * CanvasHeight);
        lv_obj_t *canvas = lv_canvas_create(display.screen);
        lv_canvas_set_buffer(canvas, buf, CanvasWidth, CanvasHeight, LV_IMG_CF_TRUE_COLOR);
        lv_canvas_fill_bg(canvas, lv_color_hex3(0xFFF), LV_OPA_COVER);
        lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 10, 80 * index + 40);
        return canvas;
    }

    lv_obj_t *CreateVoltageLabel(int index) const {
        lv_obj_t *label = lv_label_create(display.screen);
        lv_obj_set_width(label, 240);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 80 * index + 15);
        return label;
    }

    void UpdateMeter(int index, const MeterBus& meter) {
        char buffer[48]{};
        snprintf(buffer, sizeof(buffer), "-> %.2fV %dmA %dmW",
            meter.voltage,
            static_cast<int>(abs(meter.current * 1000)),
            static_cast<int>(meter.power * 1000));
        lv_obj_t* canvas = nullptr;
        SlidingBuffer* meter_buffer = nullptr;
        switch (index) {
            case 0: {
                lv_label_set_text(dc_voltage, buffer);
                canvas = dc_canvas;
                meter_buffer = &dc_buffer;
                break;
            }
            case 1: {
                lv_label_set_text(usb2_voltage, buffer);
                canvas = usb2_canvas;
                meter_buffer = &usb2_buffer;
                break;
            }
            case 2: {
                lv_label_set_text(usb1_voltage, buffer);
                canvas = usb1_canvas;
                meter_buffer = &usb1_buffer;
                break;
            }
            default: { }
        }
        if (canvas != nullptr && meter_buffer != nullptr) {
            meter_buffer->Push(meter.GetPoint());
            lv_canvas_fill_bg(canvas, lv_color_hex3(0xFFF), LV_OPA_COVER);
            for (auto i = meter_buffer->start; i < meter_buffer->start + meter_buffer->window; i++) {
                const auto& point = meter_buffer->values[i];
                auto y = CanvasHeight - 1 - point.VoltageY(CanvasHeight);
                if (y <= 0 || y >= CanvasHeight) {
                    ESP_LOGE("DisplayUi", "Bad voltage Y:%d V:%f", y, point.voltage);
                    continue;
                }
                lv_canvas_set_px(canvas, i - meter_buffer->start, y, lv_color_hex3(0xF00));
                y = CanvasHeight - 1 - point.CurrentY(CanvasHeight);
                if (y <= 0 || y >= CanvasHeight) {
                    ESP_LOGE("DisplayUi", "Bad current Y:%d I:%f", y, point.current);
                    continue;
                }
                lv_canvas_set_px(canvas, i - meter_buffer->start, CanvasHeight - 1 - point.CurrentY(CanvasHeight), lv_color_hex3(0x0F0));
            }
        }
    }
};

extern "C" void app_main(void) {
    hal::I2CBus i2cBus{34, 35};
    MeterBus usb1{i2cBus, 0x40, 33, "USB-1"};
    MeterBus usb2{i2cBus, 0x44, 36, "USB-2"};
    MeterBus dc{i2cBus, 0x41, 37, "DC Jack"};

    DisplayUi display{};

    usb1.Enable();
    usb2.Enable();

    struct Param {
        std::array<MeterBus*, 3> buses;
        DisplayUi* display;
    } param {{&usb1, &usb2, &dc}, &display };

    lv_timer_create([](lv_timer_t* timer) {
        Param& param = *static_cast<Param*>(timer->user_data);
        for (auto i = 0; i < param.buses.size(); i++) {
            param.display->UpdateMeter(i, *param.buses[i]);
        }
    }, 20, &param);

    auto statsTimer = xTimerCreate("Stats", pdMS_TO_TICKS(5000), true, nullptr, [](TimerHandle_t) {
        auto freeHeap = esp_get_free_heap_size();
        ESP_LOGI("Stats", "heap free:%.2fkB", static_cast<float>(freeHeap) / 1024.);
    });
    xTimerStart(statsTimer, 0);

    while (true) {
        usb1.Update();
        usb2.Update();
        dc.Update();
        usleep(50000);
    }
}
