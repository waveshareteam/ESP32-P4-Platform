/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "factory_ui.hpp"
#include <cstdio>
#include <initializer_list>

namespace factory_ui {
lv_obj_t *plain(lv_obj_t *parent)
{
    auto *object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE);
    return object;
}

lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color)
{
    auto *object = lv_label_create(parent);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_text_color(object, lv_color_hex(color), 0);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text(object, text ? text : "");
    return object;
}

static lv_obj_t *card(lv_obj_t *parent, int height)
{
    auto *object = plain(parent);
    lv_obj_set_size(object, lv_pct(100), height);
    lv_obj_set_style_radius(object, 24, 0);
    lv_obj_set_style_bg_color(object, lv_color_hex(CARD), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(object, 24, 0);
    return object;
}

static void pressable(lv_obj_t *object)
{
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(object, lv_color_hex(PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(object, 2, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(object, lv_color_hex(ACCENT), LV_STATE_PRESSED);
    lv_obj_set_style_opa(object, LV_OPA_40, LV_STATE_DISABLED);
}

static lv_obj_t *root(lv_obj_t *parent)
{
    auto *object = plain(parent);
    lv_obj_set_size(object, WIDTH, HEIGHT);
    lv_obj_set_style_radius(object, 0, 0);
    lv_obj_set_style_bg_color(object, lv_color_hex(BACKGROUND), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE);
    return object;
}

Page page(lv_obj_t *parent, const char *title, lv_event_cb_t back, void *context)
{
    Page result;
    result.root = root(parent);
    int title_x = GUTTER;
    if (back) {
        result.back = card(result.root, 60);
        lv_obj_set_width(result.back, 64);
        lv_obj_set_style_pad_all(result.back, 0, 0);
        lv_obj_set_style_radius(result.back, 18, 0);
        lv_obj_set_pos(result.back, GUTTER, STATUS_HEIGHT + 12);
        pressable(result.back);
        lv_obj_add_event_cb(result.back, back, LV_EVENT_CLICKED, context);
        lv_obj_center(label(result.back, LV_SYMBOL_LEFT, &lv_font_montserrat_30));
        title_x += 84;
    }
    auto *heading = label(result.root, title, &lv_font_montserrat_40);
    lv_obj_set_pos(heading, title_x, STATUS_HEIGHT + 18);
    lv_obj_set_width(heading, WIDTH - title_x - GUTTER);
    lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);

    result.content = plain(result.root);
    lv_obj_set_size(result.content, WIDTH - 2 * GUTTER, HEIGHT - CONTENT_TOP - GESTURE_INSET - 16);
    lv_obj_set_pos(result.content, GUTTER, CONTENT_TOP);
    lv_obj_add_flag(result.content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(result.content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(result.content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(result.content, lv_color_hex(HINT), LV_PART_SCROLLBAR);
    lv_obj_set_style_width(result.content, 4, LV_PART_SCROLLBAR);
    lv_obj_set_flex_flow(result.content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(result.content, 16, 0);
    lv_obj_set_style_pad_bottom(result.content, 24, 0);
    return result;
}

static void image(lv_obj_t *parent, const void *source, int size)
{
    if (!source) return;
    if (lv_image_src_get_type(source) == LV_IMAGE_SRC_SYMBOL) {
        auto *icon = label(parent, static_cast<const char *>(source), &lv_font_montserrat_48, ACCENT);
        lv_obj_center(icon);
        return;
    }
    auto *object = lv_image_create(parent);
    lv_image_set_src(object, source);
    const auto *descriptor = static_cast<const lv_image_dsc_t *>(source);
    const unsigned longest = descriptor->header.w > descriptor->header.h ? descriptor->header.w : descriptor->header.h;
    if (longest) lv_image_set_scale(object, size * LV_SCALE_NONE / longest);
    lv_obj_center(object);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE);
}

lv_obj_t *section(lv_obj_t *parent, const char *title)
{
    auto *heading = label(parent, title, &lv_font_montserrat_24, HINT);
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_height(heading, 44);
    return heading;
}

lv_obj_t *note(lv_obj_t *parent, const char *text)
{
    auto *object = label(parent, text, &lv_font_montserrat_24, HINT);
    lv_obj_set_width(object, lv_pct(100));
    lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
    return object;
}

Row row(lv_obj_t *parent, const void *icon_source, const char *title, const char *value,
        lv_event_cb_t clicked, void *context, intptr_t id)
{
    Row result;
    result.object = card(parent, 112);
    lv_obj_set_flex_flow(result.object, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(result.object, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(result.object, 20, 0);
    if (icon_source) {
        auto *icon = plain(result.object);
        lv_obj_set_size(icon, 52, 52);
        image(icon, icon_source, 48);
    }
    auto *texts = plain(result.object);
    lv_obj_set_flex_grow(texts, 1);
    lv_obj_set_height(texts, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(texts, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(texts, 8, 0);
    result.title = label(texts, title, &lv_font_montserrat_30);
    lv_obj_set_width(result.title, lv_pct(100));
    lv_label_set_long_mode(result.title, LV_LABEL_LONG_DOT);
    result.value = label(texts, value, &lv_font_montserrat_22, HINT);
    lv_obj_set_width(result.value, lv_pct(100));
    lv_label_set_long_mode(result.value, LV_LABEL_LONG_DOT);
    if (clicked) {
        pressable(result.object);
        lv_obj_set_user_data(result.object, reinterpret_cast<void *>(id));
        lv_obj_add_event_cb(result.object, clicked, LV_EVENT_CLICKED, context);
        label(result.object, LV_SYMBOL_RIGHT, &lv_font_montserrat_24, HINT);
    }
    return result;
}

static ControlPage level(lv_obj_t *parent, const char *title, const char *caption, const char *symbol, int value,
                         int minimum, int maximum, lv_event_cb_t back, lv_event_cb_t changed, void *context)
{
    ControlPage result;
    result.page = page(parent, title, back, context);
    auto *panel = card(result.page.content, 272);
    lv_obj_set_style_pad_all(panel, 30, 0);
    auto *icon = label(panel, symbol, &lv_font_montserrat_36, ACCENT);
    lv_obj_set_pos(icon, 0, 14);
    auto *name = label(panel, caption, &lv_font_montserrat_30);
    lv_obj_set_pos(name, 56, 20);
    auto *number = label(panel, "", &lv_font_montserrat_48);
    lv_obj_align(number, LV_ALIGN_TOP_RIGHT, 0, 0);
    auto *slider = lv_slider_create(panel);
    lv_obj_set_size(slider, lv_pct(100), 24);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_bg_color(slider, lv_color_hex(PRESSED), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 12, LV_PART_KNOB);
    lv_obj_set_ext_click_area(slider, 20);
    lv_slider_set_range(slider, minimum, maximum);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    lv_label_set_text_fmt(number, "%d%%", static_cast<int>(lv_slider_get_value(slider)));
    auto *low = label(panel, "", &lv_font_montserrat_22, HINT);
    lv_label_set_text_fmt(low, "%d%%", minimum);
    lv_obj_align(low, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    auto *high = label(panel, "", &lv_font_montserrat_22, HINT);
    lv_label_set_text_fmt(high, "%d%%", maximum);
    lv_obj_align(high, LV_ALIGN_BOTTOM_RIGHT, 0, -4);
    lv_obj_add_event_cb(slider, changed, LV_EVENT_VALUE_CHANGED, context);
    result.control = {slider, number};
    return result;
}

ControlPage display(lv_obj_t *parent, int brightness, lv_event_cb_t back, lv_event_cb_t changed, void *context)
{
    return level(parent, "Display", "Brightness", LV_SYMBOL_IMAGE, brightness, 20, 100, back, changed, context);
}

ControlPage sound(lv_obj_t *parent, int volume, lv_event_cb_t back, lv_event_cb_t changed, void *context)
{
    return level(parent, "Sound", "Volume", LV_SYMBOL_VOLUME_MAX, volume, 0, 95, back, changed, context);
}

lv_obj_t *PlayerView::create(lv_obj_t *parent, lv_event_cb_t action, void *context)
{
    const auto view = page(parent, "Music", nullptr, nullptr);
    auto *player = card(view.content, 310);
    auto *summary = plain(player);
    lv_obj_set_size(summary, lv_pct(100), 116);
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(summary, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(summary, 22, 0);

    auto *art = plain(summary);
    lv_obj_set_size(art, 116, 116);
    lv_obj_set_style_radius(art, 18, 0);
    lv_obj_set_style_bg_color(art, lv_color_hex(0x1B3553), 0);
    lv_obj_set_style_bg_opa(art, LV_OPA_COVER, 0);
    lv_obj_center(label(art, LV_SYMBOL_AUDIO, &lv_font_montserrat_48, ACCENT));

    auto *metadata = plain(summary);
    lv_obj_set_flex_grow(metadata, 1);
    lv_obj_set_height(metadata, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(metadata, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(metadata, 6, 0);
    _title = label(metadata, "", &lv_font_montserrat_40);
    lv_obj_set_width(_title, lv_pct(100));
    lv_label_set_long_mode(_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    _track = label(metadata, "", &lv_font_montserrat_24, HINT);

    auto *readout = plain(player);
    lv_obj_set_size(readout, lv_pct(100), 52);
    lv_obj_set_y(readout, 128);
    lv_obj_set_flex_flow(readout, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(readout, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    _state = label(readout, "Ready to play", &lv_font_montserrat_24, HINT);
    _elapsed = label(readout, "00:00", &lv_font_montserrat_28);

    auto *controls = plain(player);
    lv_obj_set_size(controls, lv_pct(100), 80);
    lv_obj_align(controls, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(controls, 12, 0);
    const auto button = [controls, action, context](const char *symbol, PlayerAction id, bool primary = false) {
        auto *surface = plain(controls);
        lv_obj_set_flex_grow(surface, primary ? 2 : 1);
        lv_obj_set_height(surface, lv_pct(100));
        lv_obj_set_style_radius(surface, 16, 0);
        lv_obj_set_style_bg_color(surface, lv_color_hex(primary ? ACCENT : 0x262936), 0);
        lv_obj_set_style_bg_opa(surface, LV_OPA_COVER, 0);
        pressable(surface);
        lv_obj_set_user_data(surface, reinterpret_cast<void *>(static_cast<intptr_t>(id)));
        lv_obj_add_event_cb(surface, action, LV_EVENT_CLICKED, context);
        auto *icon = label(surface, symbol, &lv_font_montserrat_32);
        lv_obj_center(icon);
        return icon;
    };
    button(LV_SYMBOL_PREV, PlayerAction::PREVIOUS);
    _play = button(LV_SYMBOL_PLAY, PlayerAction::PLAY_PAUSE, true);
    button(LV_SYMBOL_NEXT, PlayerAction::NEXT);

    section(view.content, "Tracks");
    _tracks = plain(view.content);
    lv_obj_set_size(_tracks, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(_tracks, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(_tracks, 12, 0);
    return view.root;
}

void PlayerView::add_track(const char *name, lv_event_cb_t action, void *context)
{
    const auto index = lv_obj_get_child_count(_tracks);
    auto *track = card(_tracks, 104);
    lv_obj_set_flex_flow(track, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(track, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(track, 20, 0);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x1B3553), LV_STATE_CHECKED);
    pressable(track);
    lv_obj_set_user_data(track, reinterpret_cast<void *>(static_cast<uintptr_t>(index)));
    lv_obj_add_event_cb(track, action, LV_EVENT_CLICKED, context);

    auto *number = label(track, "", &lv_font_montserrat_24, HINT);
    lv_obj_set_width(number, 48);
    lv_label_set_text_fmt(number, "%02u", static_cast<unsigned>(index + 1));
    auto *title = label(track, name, &lv_font_montserrat_28);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_flex_grow(title, 1);
    label(track, LV_SYMBOL_PLAY, &lv_font_montserrat_24, ACCENT);
}

void PlayerView::title(const char *name, int index, int count)
{
    lv_label_set_text(_title, name);
    lv_label_set_text_fmt(_track, "Track %d of %d", index + 1, count);
    if (_selected_track) lv_obj_remove_state(_selected_track, LV_STATE_CHECKED);
    _selected_track = lv_obj_get_child(_tracks, index);
    if (_selected_track) lv_obj_add_state(_selected_track, LV_STATE_CHECKED);
}
void PlayerView::playing(bool active)
{
    lv_label_set_text(_play, active ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    lv_label_set_text(_state, active ? "Playing" : "Paused");
}
void PlayerView::message(const char *text) { lv_label_set_text(_state, text); }
void PlayerView::elapsed(uint32_t seconds)
{
    lv_label_set_text_fmt(_elapsed, "%02u:%02u", static_cast<unsigned>(seconds / 60), static_cast<unsigned>(seconds % 60));
}
void PlayerView::clear()
{
    _title = _track = _state = _elapsed = _play = _tracks = _selected_track = nullptr;
}
} // namespace factory_ui
