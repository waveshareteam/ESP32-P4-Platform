/*
 * OV5647 MIPI-CSI camera preview for Waveshare ESP32-P4-Platform displays.
 *
 * The SCCB control channel reuses the display I2C bus. The MIPI-CSI lanes are
 * board-wired; this sketch intentionally does not allocate a second I2C bus.
 */
#ifndef BOARD_HAS_PSRAM
#error "This program requires PSRAM enabled (enable PSRAM in the Tools menu)"
#endif

#include <Arduino_GFX_Library.h>
#include <ESP_Video.h>
#include <Waveshare_Arduino_Logging.h>

#ifndef CURRENT_SCREEN
#define CURRENT_SCREEN SCREEN_10_1_DSI_TOUCH_A
#endif
#include "displays_config.h"

ESPVideoClass video;
ESPVideoCaptureDevClass capture_dev;
constexpr size_t kCaptureBufferCount = 2;

Arduino_ESP32DSIPanel *dsipanel = new Arduino_ESP32DSIPanel(
  display_cfg.hsync_pulse_width, display_cfg.hsync_back_porch,
  display_cfg.hsync_front_porch, display_cfg.vsync_pulse_width,
  display_cfg.vsync_back_porch, display_cfg.vsync_front_porch,
  display_cfg.prefer_speed, display_cfg.lane_bit_rate);
Arduino_DSI_Display *gfx = new Arduino_DSI_Display(
  display_cfg.width, display_cfg.height, dsipanel, 0, true, -1,
  display_cfg.init_cmds, display_cfg.init_cmds_size);

static bool initCamera(DEV_I2C_Port port) {
  ESPVideoCamConfigClass cam_config;
  if (!cam_config.begin(port.bus, EXAMPLE_I2C_MASTER_FREQUENCY)) {
    Serial.println("camera SCCB configuration failed");
    return false;
  }

  ESPVideoCSIConfigClass csi_config;
  if (!csi_config.begin(cam_config) || !video.begin(csi_config) ||
      !capture_dev.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, kCaptureBufferCount) ||
      !capture_dev.setFormat(ESP_VIDEO_FORMAT_RGB565) ||
      !capture_dev.startCapture()) {
    Serial.println("camera pipeline initialization failed");
    return false;
  }
  return true;
}

void setup() {
  waveshare::logging::beginSerialLog();
  DEV_I2C_Port port = DEV_I2C_Init();
  if (!display_init(port)) {
    Serial.println("display initialization failed");
    return;
  }
  set_display_backlight(port, 255);
  if (!gfx->begin()) {
    Serial.println("display begin failed");
    return;
  }
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE);
  gfx->println("Camera preview starting...");
  if (!initCamera(port)) {
    gfx->setTextColor(RGB565_RED);
    gfx->println("Camera init failed");
  }
}

void loop() {
  if (!capture_dev.isOpened() || !capture_dev.isCaptureStarted()) {
    delay(250);
    return;
  }
  ESPVideoBufferClass buffer = capture_dev.captureBuffer();
  if (!buffer.valid() || buffer.formatType() != ESP_VIDEO_FORMAT_RGB565) {
    delay(5);
    return;
  }

  const uint32_t width = buffer.getWidth();
  const uint32_t height = buffer.getHeight();
  if (width == 0 || height == 0) {
    return;
  }
  const int16_t source_x = width > gfx->width() ? (width - gfx->width()) / 2 : 0;
  const int16_t destination_x = width < gfx->width() ? (gfx->width() - width) / 2 : 0;
  const int16_t destination_y = height < gfx->height() ? (gfx->height() - height) / 2 : 0;
  const int16_t draw_width = width <= gfx->width() ? width : gfx->width();
  const int16_t draw_height = height <= gfx->height() ? height : gfx->height();
  const uint16_t *pixels = reinterpret_cast<const uint16_t *>(buffer.data());
  for (int16_t row = 0; row < draw_height; ++row) {
    gfx->draw16bitRGBBitmap(destination_x, destination_y + row,
                            const_cast<uint16_t *>(pixels + (size_t)row * width + source_x),
                            draw_width, 1);
  }
}
