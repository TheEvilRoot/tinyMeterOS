#include "hal_display.hh"

#include <esp_lcd_io_i2c.h>
#include <esp_lcd_io_spi.h>
#include <esp_lcd_panel_dev.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_ssd1306.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_types.h>
#include <esp_lvgl_port.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>

#include "hal_pin.hh"

hal::Display::Display() {
  spi_bus_config_t buscfg{};
  buscfg.sclk_io_num = 4;
  buscfg.mosi_io_num = 5;
  buscfg.miso_io_num = -1;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 280 * 240;
  buscfg.flags = 0;
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED)); // Enable the DMA feature

  esp_lcd_panel_io_handle_t io_handle{};
  esp_lcd_panel_io_spi_config_t io_config{};
  io_config.dc_gpio_num = 2;
  io_config.cs_gpio_num = 3;
  io_config.pclk_hz = SPI_MASTER_FREQ_80M;
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;
  io_config.spi_mode = 3;
  io_config.trans_queue_depth = 10;
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle));

  esp_lcd_panel_dev_config_t panel_config{};
  panel_config.reset_gpio_num = 6;
  panel_config.data_endian = LCD_RGB_DATA_ENDIAN_LITTLE;
  panel_config.bits_per_pixel = 16;
  panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
  // Create LCD panel handle for ST7789, with the SPI IO device handle
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 20));
  lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_port_init(&lvgl_cfg);
  lvgl_port_display_cfg_t disp_cfg{};
  disp_cfg.io_handle = io_handle,
  disp_cfg.panel_handle = panel_handle,
  disp_cfg.buffer_size = 280 * 240,
  disp_cfg.double_buffer = true,
  disp_cfg.hres = 240,
  disp_cfg.vres = 280,
  disp_cfg.monochrome = false,
  disp_cfg.rotation = {
    .swap_xy = false,
    .mirror_x = true,
    .mirror_y = true,
  };
  display = lvgl_port_add_disp(&disp_cfg);
  screen = lv_disp_get_scr_act(display);
}

void hal::Display::Backlight(bool on) {
    blk.SetState(on);
    printf("I hal:Display: Backlight set to %d\n", on);
    backlight = on;
}
