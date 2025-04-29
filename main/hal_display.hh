#ifndef NFCSWITCHOS_HAL_DISPLAY_HH
#define NFCSWITCHOS_HAL_DISPLAY_HH

#include <esp_lcd_types.h>
#include <lvgl.h>

#include "hal_pin.hh"

namespace hal {
  struct Display {
    Pin blk{1, PinMode::kOutput};
    esp_lcd_panel_handle_t panel_handle{};
    lv_obj_t* screen{};
    lv_disp_t* display{};

    bool backlight{false};

    Display();

    void Backlight(bool on);
  };
}


#endif //NFCSWITCHOS_HAL_DISPLAY_HH
