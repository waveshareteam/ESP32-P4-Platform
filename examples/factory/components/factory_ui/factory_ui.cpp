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
    auto *header = result.root;
    if (back) {
        result.back = card(result.root, 64);
        lv_obj_set_width(result.back, LV_SIZE_CONTENT);
        lv_obj_set_style_radius(result.back, LV_RADIUS_CIRCLE, 0);
        lv_obj_align(result.back, LV_ALIGN_TOP_MID, 0, STATUS_HEIGHT + 12);
        pressable(result.back);
        lv_obj_set_ext_click_area(result.back, 10);
        lv_obj_add_event_cb(result.back, back, LV_EVENT_CLICKED, context);
        header = result.back;
    }
    auto *heading = label(header, title, back ? &lv_font_montserrat_30 : &lv_font_montserrat_40);
    if (back) {
        lv_label_set_text_fmt(heading, LV_SYMBOL_LEFT "  %s", title);
        lv_obj_center(heading);
    } else {
        lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, STATUS_HEIGHT + 20);
    }

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

ControlPage level(lv_obj_t *parent, const char *title, const char *caption, const char *symbol, int value,
                  int minimum, int maximum, lv_event_cb_t back, lv_event_cb_t changed, void *context)
{
    ControlPage result;
    result.page = page(parent, title, back, context);
    auto *dial = plain(result.page.content);
    lv_obj_set_size(dial, lv_pct(100), WIDTH - 2 * GUTTER);
    lv_obj_set_style_margin_top(dial, 56, 0);
    auto *arc = lv_arc_create(dial);
    const int diameter = WIDTH - 4 * GUTTER;
    lv_obj_set_size(arc, diameter, diameter);
    lv_obj_center(arc);
    lv_arc_set_bg_angles(arc, 310, 230);
    lv_arc_set_range(arc, minimum, maximum);
    lv_arc_set_value(arc, value);
    lv_obj_set_style_arc_width(arc, 24, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 24, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x2A2B35), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 5, LV_PART_KNOB);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_event_cb(arc, changed, LV_EVENT_VALUE_CHANGED, context);
    auto *icon = label(dial, symbol, &lv_font_montserrat_48, ACCENT);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -106);
    auto *level_value = label(dial, "", &lv_font_montserrat_48);
    lv_label_set_text_fmt(level_value, "%d%%", static_cast<int>(lv_arc_get_value(arc)));
    lv_obj_set_style_transform_scale(level_value, 400, 0);
    lv_obj_set_style_transform_pivot_x(level_value, lv_pct(50), 0);
    lv_obj_set_style_transform_pivot_y(level_value, lv_pct(50), 0);
    lv_obj_center(level_value);
    auto *name = label(dial, caption, &lv_font_montserrat_28, HINT);
    lv_obj_align(name, LV_ALIGN_CENTER, 0, 104);
    result.control = {arc, level_value};
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
    constexpr int disc_size = WIDTH * 53 / 100;
    constexpr int disc_center_y = HEIGHT * 37 / 100;
    constexpr uint32_t dim = 0xA0A3B1;
    auto *view = root(parent);
    lv_obj_set_style_bg_color(view, lv_color_hex(0x0B0B12), 0);

    const auto bind = [action, context](lv_obj_t *object, PlayerAction id) {
        lv_obj_set_user_data(object, reinterpret_cast<void *>(static_cast<intptr_t>(id)));
        lv_obj_add_event_cb(object, action, LV_EVENT_CLICKED, context);
    };
    const auto button = [&bind](lv_obj_t *container, const char *symbol, int size, PlayerAction id) {
        auto *surface = plain(container);
        lv_obj_set_size(surface, size, size);
        lv_obj_set_style_radius(surface, LV_RADIUS_CIRCLE, 0);
        lv_obj_add_flag(surface, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(surface, lv_color_hex(0x2A2B36), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(surface, LV_OPA_COVER, LV_STATE_PRESSED);
        bind(surface, id);
        lv_obj_center(label(surface, symbol, &lv_font_montserrat_36));
        return surface;
    };
    auto *list = button(view, LV_SYMBOL_LIST, 80, PlayerAction::TRACKS);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, STATUS_HEIGHT + 24);

    auto *disc = plain(view);
    lv_obj_set_size(disc, disc_size, disc_size);
    lv_obj_align(disc, LV_ALIGN_TOP_MID, 0, disc_center_y - disc_size / 2);
    lv_obj_set_style_radius(disc, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(disc, lv_color_hex(0x1A1B24), 0);
    lv_obj_set_style_bg_opa(disc, LV_OPA_COVER, 0);
    lv_obj_add_flag(disc, LV_OBJ_FLAG_CLICKABLE);
    bind(disc, PlayerAction::PLAY_PAUSE);
    for (int size = disc_size - 32; size > disc_size / 3; size -= 44) {
        auto *groove = plain(disc);
        lv_obj_set_size(groove, size, size);
        lv_obj_center(groove);
        lv_obj_set_style_radius(groove, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(groove, 2, 0);
        lv_obj_set_style_border_color(groove, lv_color_hex(0x2C2D3A), 0);
    }
    lv_obj_center(label(disc, LV_SYMBOL_AUDIO, &lv_font_montserrat_48, ACCENT));

    auto *metadata = plain(view);
    lv_obj_set_size(metadata, WIDTH - 2 * GUTTER, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(metadata, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(metadata, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(metadata, 16, 0);
    lv_obj_align(metadata, LV_ALIGN_TOP_MID, 0, disc_center_y + disc_size / 2 + 40);
    _title = label(metadata, "", &lv_font_montserrat_44);
    lv_obj_set_width(_title, lv_pct(100));
    lv_obj_set_style_text_align(_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    _track = label(metadata, "", &lv_font_montserrat_28, dim);
    _elapsed = label(metadata, "00:00", &lv_font_montserrat_24, dim);
    _state = label(metadata, "Ready to play", &lv_font_montserrat_24, dim);

    auto *controls = plain(view);
    lv_obj_set_size(controls, LV_SIZE_CONTENT, 120);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(controls, 40, 0);
    lv_obj_align(controls, LV_ALIGN_BOTTOM_MID, 0, -GESTURE_INSET - 74);
    button(controls, LV_SYMBOL_PREV, 88, PlayerAction::PREVIOUS);
    auto *play = button(controls, LV_SYMBOL_PLAY, 120, PlayerAction::PLAY_PAUSE);
    lv_obj_set_style_bg_color(play, lv_color_hex(0x4F8BFF), 0);
    lv_obj_set_style_bg_opa(play, LV_OPA_COVER, 0);
    _play = lv_obj_get_child(play, 0);
    button(controls, LV_SYMBOL_NEXT, 88, PlayerAction::NEXT);
    return view;
}

void PlayerView::title(const char *name, int index, int count)
{
    lv_label_set_text(_title, name);
    lv_label_set_text_fmt(_track, "Track %d of %d", index + 1, count);
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
void PlayerView::clear() { _title = _track = _state = _elapsed = _play = nullptr; }
} // namespace factory_ui
