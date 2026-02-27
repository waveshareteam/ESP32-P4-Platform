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
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX Hello World example");

  DEV_I2C_Port port = DEV_I2C_Init();

  display_init(port);

  delay(1000);

  int numCols = (display_cfg.width / 12) - 1;
  int numRows = display_cfg.height / 16;

  // Init Display
    if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  gfx->setTextColor(GREEN);
  for (int x = 0; x < numCols; x++) {
    gfx->setCursor(16 + x * 12, 4);
    gfx->print(x);
  }
  gfx->setTextColor(BLUE);
  for (int y = 0; y < numRows; y++) {
    gfx->setCursor(4, 16 + y * 16);
    gfx->print(y);
  }

  char c = 0;
  for (int y = 0; y < numRows; y++) {
    for (int x = 0; x < numCols; x++) {
      gfx->drawChar(16 + x * 12, 16 + y * 16, c++, WHITE, BLACK);
    }
  }

  delay(5000);  // 5 seconds
}

void loop() {
}