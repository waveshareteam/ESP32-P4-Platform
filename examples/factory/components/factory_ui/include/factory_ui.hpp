/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "bsp/display.h"
#include "lvgl.h"

// The Knob factory application's dark cards and blue controls, laid out for a
// rectangular portrait display. No circular safe area or rotary input is used.
namespace factory_ui {
constexpr int WIDTH = BSP_LCD_H_RES;
constexpr int HEIGHT = BSP_LCD_V_RES;
constexpr int STATUS_HEIGHT = 64;
constexpr int GESTURE_INSET = 30;
constexpr int SWIPE_DISTANCE = 50;
constexpr int SWIPE_ANGLE = 60;
constexpr int GUTTER = 32;
constexpr int CONTENT_TOP = 192;
constexpr uint32_t BACKGROUND = 0x0C0D12;
constexpr uint32_t CARD = 0x1C1D24;
constexpr uint32_t PRESSED = 0x2C2D38;
constexpr uint32_t ACCENT = 0x3D9BFF;
constexpr uint32_t TEXT = 0xFFFFFF;
constexpr uint32_t HINT = 0x8E929E;

// Full-screen media reads touch directly while the LVGL adapter is paused.
constexpr bool is_bottom_swipe_up(int start_x, int start_y, int x, int y)
{
    const int dx = x >= start_x ? x - start_x : start_x - x;
    const int up = start_y - y;
    return HEIGHT - start_y < GESTURE_INSET && up > SWIPE_DISTANCE && up > 1.73205f * dx;
}

struct Page {
    lv_obj_t *root = nullptr;
    lv_obj_t *content = nullptr;
    lv_obj_t *back = nullptr;
};

struct Row {
    lv_obj_t *object = nullptr;
    lv_obj_t *title = nullptr;
    lv_obj_t *value = nullptr;
};

struct LevelControl {
    lv_obj_t *object = nullptr;
    lv_obj_t *value = nullptr;
};
struct ControlPage {
    Page page;
    LevelControl control;
};

lv_obj_t *plain(lv_obj_t *parent);
lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color = TEXT);
Page page(lv_obj_t *parent, const char *title, lv_event_cb_t back, void *context);
lv_obj_t *section(lv_obj_t *parent, const char *title);
lv_obj_t *note(lv_obj_t *parent, const char *text);
Row row(lv_obj_t *parent, const void *icon_source, const char *title, const char *value,
        lv_event_cb_t clicked = nullptr, void *context = nullptr, intptr_t id = 0);
ControlPage display(lv_obj_t *parent, int brightness, lv_event_cb_t back, lv_event_cb_t changed, void *context);
ControlPage sound(lv_obj_t *parent, int volume, lv_event_cb_t back, lv_event_cb_t changed, void *context);

enum class PlayerAction { PREVIOUS, PLAY_PAUSE, NEXT, TRACKS };
class PlayerView {
public:
    lv_obj_t *create(lv_obj_t *parent, lv_event_cb_t action, void *context);
    void title(const char *name, int index, int count);
    void playing(bool active);
    void message(const char *text);
    void elapsed(uint32_t seconds);
    void clear();
private:
    lv_obj_t *_title = nullptr;
    lv_obj_t *_track = nullptr;
    lv_obj_t *_state = nullptr;
    lv_obj_t *_elapsed = nullptr;
    lv_obj_t *_play = nullptr;
};
} // namespace factory_ui
