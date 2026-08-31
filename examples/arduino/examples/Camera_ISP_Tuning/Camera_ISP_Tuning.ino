/*
 * OV5647 MIPI-CSI preview with non-blocking serial ISP controls for
 * Waveshare ESP32-P4-Platform displays.
 *
 * Commands: g <gain>, e <exposure-us>, a <AE target>, v/h/t <0|1>, s.
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

struct IspSettings {
  int32_t gain = 0;
  int32_t exposure = 0;
  int32_t ae_target = 0;
  int32_t vflip = -1;
  int32_t hflip = -1;
  int32_t test_pattern = -1;
} settings;

static bool applySetting(char operation, int32_t value) {
  bool accepted = false;
  switch (operation) {
    case 'g': accepted = capture_dev.setSensorGain(value); break;
    case 'e': accepted = capture_dev.setSensorExposureTime(value); break;
    case 'a': accepted = capture_dev.setSensorAETargetLevel(value); break;
    case 'v': accepted = capture_dev.setSensorVFlip(value != 0); break;
    case 'h': accepted = capture_dev.setSensorHFlip(value != 0); break;
    case 't': accepted = capture_dev.setSensorTestPattern(value != 0); break;
    default: return false;
  }
  if (!accepted) return false;

  switch (operation) {
    case 'g': settings.gain = value; break;
    case 'e': settings.exposure = value; break;
    case 'a': settings.ae_target = value; break;
    case 'v': settings.vflip = value; break;
    case 'h': settings.hflip = value; break;
    case 't': settings.test_pattern = value; break;
  }
  return true;
}

static void printSettings() {
  Serial.printf("gain=%ld exposure_us=%ld ae_target=%ld vflip=%ld hflip=%ld test_pattern=%ld\n",
                (long)settings.gain, (long)settings.exposure, (long)settings.ae_target,
                (long)settings.vflip, (long)settings.hflip, (long)settings.test_pattern);
}

static void handleSerial() {
  static String command;
  int available = Serial.available();
  while (available-- > 0) {
    const char character = static_cast<char>(Serial.read());
    if (character == '\n' || character == '\r') {
      command.trim();
      if (command.length() != 0) {
        const char operation = command[0];
        const long value = command.length() > 2 ? command.substring(2).toInt() : 0;
        if (operation == 's') {
          printSettings();
          command = "";
          continue;
        }
        if (!applySetting(operation, value)) {
          Serial.println("setting rejected or command unknown");
        } else {
          printSettings();
        }
      }
      command = "";
    } else if (command.length() < 32) {
      command += character;
    }
  }
}

static bool initCamera(DEV_I2C_Port port) {
  ESPVideoCamConfigClass cam_config;
  if (!cam_config.begin(port.bus, EXAMPLE_I2C_MASTER_FREQUENCY)) {
    Serial.println("camera SCCB configuration failed");
    return false;
  }
  ESPVideoCSIConfigClass csi_config;
  return csi_config.begin(cam_config) && video.begin(csi_config) &&
         capture_dev.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, kCaptureBufferCount) &&
         capture_dev.setFormat(ESP_VIDEO_FORMAT_RGB565) && capture_dev.startCapture();
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
  if (!initCamera(port)) {
    Serial.println("camera pipeline initialization failed");
    return;
  }
  Serial.println("ISP tuning ready: g/e/a/v/h/t/s");
  printSettings();
}

void loop() {
  handleSerial();
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
  if (width == 0 || height == 0) return;
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
  gfx->setTextColor(RGB565_BLACK, RGB565_WHITE);
  gfx->setCursor(4, 4);
  gfx->printf("gain=%ld exp=%ld ae=%ld", (long)settings.gain,
              (long)settings.exposure, (long)settings.ae_target);
}
