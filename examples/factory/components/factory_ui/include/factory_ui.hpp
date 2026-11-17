/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "lvgl.h"

// The Knob factory application's dark cards and blue controls, laid out for a
// rectangular portrait display. No circular safe area or rotary input is used.
namespace factory_ui {
constexpr int WIDTH = 800;
constexpr int HEIGHT = 1280;
constexpr int STATUS_HEIGHT = 64;
constexpr int NAVIGATION_HEIGHT = 80;
constexpr int GUTTER = 32;
constexpr int CONTENT_TOP = 192;
constexpr uint32_t BACKGROUND = 0x0C0D12;
constexpr uint32_t CARD = 0x1C1D24;
constexpr uint32_t PRESSED = 0x2C2D38;
constexpr uint32_t ACCENT = 0x3D9BFF;
constexpr uint32_t TEXT = 0xFFFFFF;
constexpr uint32_t HINT = 0x8E929E;

struct Page {
    lv_obj_t *root = nullptr;
    lv_obj_t *content = nullptr;
    lv_obj_t *back = nullptr;
};

struct Tile {
    int id;
    const char *title;
    const char *description;
    const void *image;
};

struct Row {
    lv_obj_t *object = nullptr;
    lv_obj_t *value = nullptr;
};

struct Slider {
    lv_obj_t *object = nullptr;
    lv_obj_t *value = nullptr;
};
struct ControlPage {
    Page page;
    Slider control;
};

lv_obj_t *plain(lv_obj_t *parent);
lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color = TEXT);
Page page(lv_obj_t *parent, const char *title, lv_event_cb_t back, void *context);
lv_obj_t *home(lv_obj_t *parent, const Tile *tiles, size_t count, lv_event_cb_t activate, void *context);
void section(lv_obj_t *parent, const char *title);
lv_obj_t *note(lv_obj_t *parent, const char *text);
Row row(lv_obj_t *parent, const char *symbol, const char *title, const char *value,
        lv_event_cb_t clicked = nullptr, void *context = nullptr, intptr_t id = 0);
Slider slider(lv_obj_t *parent, const char *title, const char *description, int value, int minimum,
              int maximum, lv_event_cb_t changed, void *context);
Page settings(lv_obj_t *parent, const char *wifi, lv_event_cb_t back, lv_event_cb_t selected, void *context);
ControlPage display(lv_obj_t *parent, int brightness, lv_event_cb_t back, lv_event_cb_t changed, void *context);
ControlPage sound(lv_obj_t *parent, int volume, lv_event_cb_t back, lv_event_cb_t changed, void *context);
Page about(lv_obj_t *parent, const char *version, lv_event_cb_t back, void *context);

enum class PlayerAction { PREVIOUS, PLAY_PAUSE, NEXT, TRACKS };
class PlayerView {
public:
    Page create(lv_obj_t *parent, const void *art, lv_event_cb_t back, lv_event_cb_t action, void *context);
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
