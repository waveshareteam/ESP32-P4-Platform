#ifndef BOARD_HAS_PSRAM
#error "Error: This program requires PSRAM enabled, please enable PSRAM option in 'Tools' menu of Arduino IDE"
#endif

#include <Arduino_GFX_Library.h>
#include "WiFi.h"

int16_t w, h, text_size, banner_height, graph_baseline, graph_height, channel_width, signal_width;

// RSSI RANGE
#define RSSI_CEILING -40
#define RSSI_FLOOR -100

// Channel color mapping from channel 1 to 14
uint16_t channel_color[] = {
  RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, MAGENTA,
  RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, MAGENTA
};

uint8_t scan_count = 0;

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

  // Init Display
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  w = gfx->width();
  h = gfx->height();
  text_size = (h < 200) ? 1 : 2;
  banner_height = text_size * 3 * 4;
  graph_baseline = h - 20;                             // minus 2 text lines
  graph_height = graph_baseline - banner_height - 30;  // minus 3 text lines
  channel_width = w / 17;
  signal_width = channel_width * 2;

  set_display_backlight(port, 255);

  // init banner
  gfx->setTextSize(text_size);
  gfx->fillScreen(BLACK);
  gfx->setTextColor(RED);
  gfx->setCursor(0, 0);
  gfx->print("ESP");
  gfx->setTextColor(WHITE);
  gfx->print(" WiFi Analyzer");
}

bool matchBssidPrefix(uint8_t *a, uint8_t *b) {
  for (uint8_t i = 0; i < 5; i++) {  // only compare first 5 bytes
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

void loop() {
  uint8_t ap_count_list[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int32_t noise_list[] = { RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR };
  int32_t peak_list[] = { RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR, RSSI_FLOOR };
  int16_t peak_id_list[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  int32_t channel;
  int16_t idx;
  int32_t rssi;
  uint8_t *bssid;
  String ssid;
  uint16_t color;
  int16_t height, offset, text_width;

  // WiFi.scanNetworks will return the number of networks found
#if defined(ESP32)
  int n = WiFi.scanNetworks(false /* async */, true /* show_hidden */, true /* passive */, 500 /* max_ms_per_chan */);
#else
  int n = WiFi.scanNetworks(false /* async */, true /* show_hidden */);
#endif

  // clear old graph
  gfx->fillRect(0, banner_height, w, h - banner_height, BLACK);
  gfx->setTextSize(1);

  if (n == 0) {
    gfx->setTextColor(WHITE);
    gfx->setCursor(0, banner_height);
    gfx->println("no networks found");
  } else {
    for (int i = 0; i < n; i++) {
      channel = WiFi.channel(i);
      idx = channel - 1;
      rssi = WiFi.RSSI(i);
      bssid = WiFi.BSSID(i);

      // channel peak stat
      if (peak_list[idx] < rssi) {
        peak_list[idx] = rssi;
        peak_id_list[idx] = i;
      }

      // check signal come from same AP
      bool duplicate_SSID = false;
      for (int j = 0; j < i; j++) {
        if ((WiFi.channel(j) == channel) && matchBssidPrefix(WiFi.BSSID(j), bssid)) {
          duplicate_SSID = true;
          break;
        }
      }

      if (!duplicate_SSID) {
        ap_count_list[idx]++;

        // noise stat
        int32_t noise = rssi - RSSI_FLOOR;
        noise *= noise;
        if (channel > 4) {
          noise_list[idx - 4] += noise;
        }
        if (channel > 3) {
          noise_list[idx - 3] += noise;
        }
        if (channel > 2) {
          noise_list[idx - 2] += noise;
        }
        if (channel > 1) {
          noise_list[idx - 1] += noise;
        }
        noise_list[idx] += noise;
        if (channel < 14) {
          noise_list[idx + 1] += noise;
        }
        if (channel < 13) {
          noise_list[idx + 2] += noise;
        }
        if (channel < 12) {
          noise_list[idx + 3] += noise;
        }
        if (channel < 11) {
          noise_list[idx + 4] += noise;
        }
      }
    }

    // plot found WiFi info
    for (int i = 0; i < n; i++) {
      channel = WiFi.channel(i);
      idx = channel - 1;
      rssi = WiFi.RSSI(i);
      color = channel_color[idx];
      height = constrain(map(rssi, RSSI_FLOOR, RSSI_CEILING, 1, graph_height), 1, graph_height);
      offset = (channel + 1) * channel_width;

      // trim rssi with RSSI_FLOOR
      if (rssi < RSSI_FLOOR) {
        rssi = RSSI_FLOOR;
      }

      // plot chart
      // gfx->drawLine(offset, graph_baseline - height, offset - signal_width, graph_baseline + 1, color);
      // gfx->drawLine(offset, graph_baseline - height, offset + signal_width, graph_baseline + 1, color);
      gfx->startWrite();
      gfx->writeEllipseHelper(offset, graph_baseline + 1, signal_width, height, 0b0011, color);
      gfx->endWrite();

      if (i == peak_id_list[idx]) {
        // Print SSID, signal strengh and if not encrypted
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) {
          ssid = WiFi.BSSIDstr(i);
        }
        text_width = (ssid.length() + 6) * 6;
        if (text_width > w) {
          offset = 0;
        } else {
          offset -= signal_width;
          if ((offset + text_width) > w) {
            offset = w - text_width;
          }
        }
        gfx->setTextColor(color);
        gfx->setCursor(offset, graph_baseline - 10 - height);
        gfx->print(ssid);
        gfx->print('(');
        gfx->print(rssi);
        gfx->print(')');
#if defined(ESP32)
        if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)
#else
        if (WiFi.encryptionType(i) == ENC_TYPE_NONE)
#endif
        {
          gfx->print('*');
        }
      }
    }
  }

  // print WiFi stat
  gfx->setTextColor(WHITE);
  gfx->setCursor(0, banner_height);
  gfx->print(n);
  gfx->print(" networks found, lesser noise channels: ");
  bool listed_first_channel = false;
  int32_t min_noise = noise_list[0];           // init with channel 1 value
  for (channel = 2; channel <= 11; channel++)  // channels 12-14 may not available
  {
    idx = channel - 1;
    log_i("min_noise: %d, noise_list[%d]: %d", min_noise, idx, noise_list[idx]);
    if (noise_list[idx] < min_noise) {
      min_noise = noise_list[idx];
    }
  }

  for (channel = 1; channel <= 11; channel++)  // channels 12-14 may not available
  {
    idx = channel - 1;
    // check channel with min noise
    if (noise_list[idx] == min_noise) {
      if (!listed_first_channel) {
        listed_first_channel = true;
      } else {
        gfx->print(", ");
      }
      gfx->print(channel);
    }
  }

  // draw graph base axle
  gfx->drawFastHLine(0, graph_baseline, gfx->width(), WHITE);
  for (channel = 1; channel <= 14; channel++) {
    idx = channel - 1;
    offset = (channel + 1) * channel_width;
    gfx->setTextColor(channel_color[idx]);
    gfx->setCursor(offset - ((channel < 10) ? 3 : 6), graph_baseline + 2);
    gfx->print(channel);
    if (ap_count_list[idx] > 0) {
      gfx->setCursor(offset - ((ap_count_list[idx] < 10) ? 9 : 12), graph_baseline + 8 + 2);
      gfx->print('{');
      gfx->print(ap_count_list[idx]);
      gfx->print('}');
    }
  }

  // Wait a bit before scanning again
  // delay(SCAN_INTERVAL);

#if defined(SCAN_COUNT_SLEEP)
  // POWER SAVING
  if (++scan_count >= SCAN_COUNT_SLEEP) {
#if defined(LCD_PWR_PIN)
    pinMode(LCD_PWR_PIN, INPUT);  // disable pin
#endif

#if defined(GFX_BL)
    pinMode(GFX_BL, INPUT);  // disable pin
#endif

#if defined(ESP32)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);
    esp_deep_sleep_start();
#else
    ESP.deepSleep(0);
#endif
  }
#endif  // defined(SCAN_COUNT_SLEEP)
}
