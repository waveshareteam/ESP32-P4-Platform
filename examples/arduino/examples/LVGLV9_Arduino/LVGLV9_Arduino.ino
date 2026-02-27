#ifndef BOARD_HAS_PSRAM
#error "Error: This program requires PSRAM enabled, please enable PSRAM option in 'Tools' menu of Arduino IDE"
#endif

#include <Arduino_GFX_Library.h>
#include "gt911.h"
#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>

static esp_lcd_touch_handle_t tp_handle = NULL;
#define MAX_TOUCH_POINTS 5

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

#define LVGL_TICK_PERIOD 5  // ms
#define DRAW_BUF_HEIGHT 50
static lv_display_t *lv_display;
static lv_indev_t *indev_touchpad;
static lv_color_t *lv_draw_buf1;
static lv_color_t *lv_draw_buf2;
static uint16_t touch_x[MAX_TOUCH_POINTS] = { 0 };
static uint16_t touch_y[MAX_TOUCH_POINTS] = { 0 };
static uint16_t touch_strength[MAX_TOUCH_POINTS] = { 0 };
static uint8_t touch_cnt = 0;
static bool touch_pressed = false;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
  lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  esp_lcd_touch_read_data(tp_handle);
  touch_pressed = esp_lcd_touch_get_coordinates(
    tp_handle, touch_x, touch_y, touch_strength, &touch_cnt, MAX_TOUCH_POINTS);

  if (touch_pressed && touch_cnt > 0) {
    data->point.x = touch_x[0];
    data->point.y = touch_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void lvglTick(void *param) {
  lv_tick_inc(LVGL_TICK_PERIOD);
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("Arduino GFX + LVGL Integration");

  delay(1000);

  DEV_I2C_Port port = DEV_I2C_Init();
  display_init(port);
  tp_handle = touch_gt911_init(port);

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }

  for (int i = 0; i <= 255; i++) {
    set_display_backlight(port, i);
    delay(3);
  }

  lv_init();

  size_t draw_buf_size = display_cfg.width * DRAW_BUF_HEIGHT;
  lv_draw_buf1 = (lv_color_t *)heap_caps_malloc(draw_buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  if (!lv_draw_buf1) {
    Serial.println("LVGL draw buffer 1 allocation failed!");
  }
  
  lv_draw_buf2 = (lv_color_t *)heap_caps_malloc(draw_buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  if (!lv_draw_buf2) {
    Serial.println("LVGL draw buffer 2 allocation failed!");
    heap_caps_free(lv_draw_buf1);
  }

  lv_display = lv_display_create(display_cfg.width, display_cfg.height);
  lv_display_set_flush_cb(lv_display, my_disp_flush);
  lv_display_set_buffers(lv_display, lv_draw_buf1, lv_draw_buf2, draw_buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

  indev_touchpad = lv_indev_create();
  lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev_touchpad, my_touchpad_read);

  //   lv_obj_t *label = lv_label_create(lv_screen_active());
  //   lv_label_set_text(label, "Hello LVGL with Arduino GFX!");
  //   lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  const esp_timer_create_args_t lvgl_timer_args = {
    .callback = &lvglTick,
    .name = "lvgl_timer"
  };
  esp_timer_handle_t lvgl_timer;
  esp_timer_create(&lvgl_timer_args, &lvgl_timer);
  esp_timer_start_periodic(lvgl_timer, LVGL_TICK_PERIOD * 1000);

  lv_display_set_dpi(lv_display, 150);
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);

  lv_demo_widgets();
  // lv_demo_benchmark();  // This stress test causes the program to hang due to arduino driver optimization issues as well as cache allocation issues
  // lv_demo_keypad_encoder();
  //   lv_demo_music();
  // lv_demo_stress();

  Serial.println("Setup complete");
}

void loop() {
  lv_timer_handler();
  delay(5);
}