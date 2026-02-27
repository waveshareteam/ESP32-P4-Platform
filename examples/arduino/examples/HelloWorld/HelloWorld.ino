#ifndef BOARD_HAS_PSRAM
#error "Error: This program requires PSRAM enabled, please enable PSRAM option in 'Tools' menu of Arduino IDE"
#endif

#include <Arduino_GFX_Library.h>

// SCREEN_10_1_DSI_TOUCH_A for 10.1-DSI-Touch-A
// SCREEN_8_DSI_TOUCH_A for 8-DSI-Touch-A
// SCREEN_7_DSI_TOUCH_A for 7-DSI-Touch-A
#ifndef CURRENT_SCREEN
#define CURRENT_SCREEN SCREEN_10_1_DSI_TOUCH_A
#endif
#include "displays_config.h"

Arduino_ESP32DSIPanel *dsipanel = new Arduino_ESP32DSIPanel(
  display_cfg.hsync_pulse_width,
  display_cfg.hsync_back_porch,
  display_cfg.hsync_front_porch,
  display_cfg.vsync_pulse_width,
  display_cfg.vsync_back_porch,
  display_cfg.vsync_front_porch,
  display_cfg.prefer_speed,
  display_cfg.lane_bit_rate);
Arduino_DSI_Display *gfx = new Arduino_DSI_Display(
  display_cfg.width,
  display_cfg.height,
  dsipanel,
  0,
  true,
  -1,
  display_cfg.init_cmds,
  display_cfg.init_cmds_size);

void setup(void) {

  Serial.begin(115200);
  Serial.println("Arduino_GFX Hello World example");

  DEV_I2C_Port port = DEV_I2C_Init();

  display_init(port);

  set_display_backlight(port, 255);

  delay(1000);

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

  gfx->setCursor(10, 10);
  gfx->setTextColor(RGB565_RED);
  gfx->println("Hello World!");

  delay(5000);  // 5 seconds
}

void loop() {
  gfx->setCursor(random(gfx->width()), random(gfx->height()));
  gfx->setTextColor(random(0xffff), random(0xffff));
  gfx->setTextSize(random(6) /* x scale */, random(6) /* y scale */, random(2) /* pixel_margin */);
  gfx->println("Hello World!");

  delay(1000);  // 1 second
}
