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
    // The home view covers the framework's default icon table. Empty space must
    // consume touches rather than activate an icon behind this view.
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE);
    return object;
}

Page page(lv_obj_t *parent, const char *title, lv_event_cb_t back, void *context)
{
    Page result;
    result.root = root(parent);
    result.back = card(result.root, 72);
    lv_obj_set_size(result.back, 72, 72);
    lv_obj_set_pos(result.back, GUTTER, STATUS_HEIGHT + 16);
    pressable(result.back);
    if (back) lv_obj_add_event_cb(result.back, back, LV_EVENT_CLICKED, context);
    auto *arrow = label(result.back, LV_SYMBOL_LEFT, &lv_font_montserrat_32, ACCENT);
    lv_obj_center(arrow);
    auto *heading = label(result.root, title, &lv_font_montserrat_40);
    lv_obj_set_width(heading, WIDTH - 2 * GUTTER - 96);
    lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(heading, GUTTER + 96, STATUS_HEIGHT + 30);

    result.content = plain(result.root);
    lv_obj_set_size(result.content, WIDTH - 2 * GUTTER, HEIGHT - CONTENT_TOP - NAVIGATION_HEIGHT - 16);
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

lv_obj_t *home(lv_obj_t *parent, const Tile *tiles, size_t count, lv_event_cb_t activate, void *context)
{
    auto *view = root(parent);
    auto *heading = label(view, "ESP32-P4", &lv_font_montserrat_48);
    lv_obj_set_pos(heading, GUTTER, STATUS_HEIGHT + 28);
    auto *subtitle = label(view, "Factory applications", &lv_font_montserrat_24, HINT);
    lv_obj_set_pos(subtitle, GUTTER, STATUS_HEIGHT + 94);
    auto *divider = plain(view);
    lv_obj_set_size(divider, WIDTH - 2 * GUTTER, 2);
    lv_obj_set_pos(divider, GUTTER, 202);
    lv_obj_set_style_bg_color(divider, lv_color_hex(PRESSED), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    auto *caption = label(view, "Applications", &lv_font_montserrat_22, HINT);
    lv_obj_set_pos(caption, GUTTER, 224);

    auto *grid = plain(view);
    lv_obj_set_size(grid, WIDTH - 2 * GUTTER, HEIGHT - 276 - NAVIGATION_HEIGHT - 62);
    lv_obj_set_pos(grid, GUTTER, 276);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(grid, 20, 0);
    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);
    for (size_t i = 0; i < count; ++i) {
        auto *tile = card(grid, 248);
        lv_obj_set_width(tile, 232);
        lv_obj_set_style_pad_all(tile, 16, 0);
        pressable(tile);
        lv_obj_set_user_data(tile, reinterpret_cast<void *>(static_cast<intptr_t>(tiles[i].id)));
        lv_obj_add_event_cb(tile, activate, LV_EVENT_CLICKED, context);
        auto *art = plain(tile);
        lv_obj_set_size(art, 128, 128);
        lv_obj_align(art, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_remove_flag(art, LV_OBJ_FLAG_CLICKABLE);
        image(art, tiles[i].image, 112);
        auto *title = label(tile, tiles[i].title, &lv_font_montserrat_28);
        lv_obj_set_width(title, 200);
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 140);
        auto *description = label(tile, tiles[i].description, &lv_font_montserrat_20, HINT);
        lv_obj_set_width(description, 200);
        lv_obj_set_style_text_align(description, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(description, LV_LABEL_LONG_DOT);
        lv_obj_align(description, LV_ALIGN_TOP_MID, 0, 182);
    }
    auto *hint = label(view, "Tap an application to open", &lv_font_montserrat_22, HINT);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -NAVIGATION_HEIGHT - 24);
    return view;
}

void section(lv_obj_t *parent, const char *title)
{
    auto *heading = label(parent, title, &lv_font_montserrat_24, HINT);
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_height(heading, 44);
}

lv_obj_t *note(lv_obj_t *parent, const char *text)
{
    auto *object = label(parent, text, &lv_font_montserrat_24, HINT);
    lv_obj_set_width(object, lv_pct(100));
    lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
    return object;
}

Row row(lv_obj_t *parent, const char *symbol, const char *title, const char *value,
        lv_event_cb_t clicked, void *context, intptr_t id)
{
    Row result;
    result.object = card(parent, 128);
    lv_obj_set_flex_flow(result.object, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(result.object, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(result.object, 20, 0);
    if (symbol) {
        auto *icon = label(result.object, symbol, &lv_font_montserrat_36, ACCENT);
        lv_obj_set_width(icon, 48);
        lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, 0);
    }
    auto *texts = plain(result.object);
    lv_obj_set_flex_grow(texts, 1);
    lv_obj_set_height(texts, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(texts, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(texts, 8, 0);
    auto *name = label(texts, title, &lv_font_montserrat_30);
    lv_obj_set_width(name, lv_pct(100));
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
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

Slider slider(lv_obj_t *parent, const char *title, const char *description, int value, int minimum,
              int maximum, lv_event_cb_t changed, void *context)
{
    auto *panel = card(parent, 308);
    lv_obj_set_style_pad_all(panel, 28, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, 22, 0);
    auto *top = plain(panel);
    lv_obj_set_size(top, lv_pct(100), 64);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    auto *name = label(top, title, &lv_font_montserrat_30);
    lv_obj_set_flex_grow(name, 1);
    Slider result;
    result.value = label(top, "", &lv_font_montserrat_48, ACCENT);
    lv_label_set_text_fmt(result.value, "%d%%", value);
    auto *detail = label(panel, description, &lv_font_montserrat_22, HINT);
    lv_obj_set_width(detail, lv_pct(100));
    lv_label_set_long_mode(detail, LV_LABEL_LONG_WRAP);
    result.object = lv_slider_create(panel);
    lv_obj_set_size(result.object, lv_pct(100), 24);
    lv_slider_set_range(result.object, minimum, maximum);
    lv_slider_set_value(result.object, value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(result.object, lv_color_hex(PRESSED), LV_PART_MAIN);
    lv_obj_set_style_bg_color(result.object, lv_color_hex(ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(result.object, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(result.object, 12, LV_PART_KNOB);
    lv_obj_set_ext_click_area(result.object, 20);
    if (changed) lv_obj_add_event_cb(result.object, changed, LV_EVENT_VALUE_CHANGED, context);
    auto *range = plain(panel);
    lv_obj_set_size(range, lv_pct(100), 28);
    lv_obj_set_flex_flow(range, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(range, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    auto *minimum_label = label(range, "", &lv_font_montserrat_20, HINT);
    auto *maximum_label = label(range, "", &lv_font_montserrat_20, HINT);
    lv_label_set_text_fmt(minimum_label, "%d%%", minimum);
    lv_label_set_text_fmt(maximum_label, "%d%%", maximum);
    return result;
}

Page settings(lv_obj_t *parent, const char *wifi, lv_event_cb_t back, lv_event_cb_t selected, void *context)
{
    auto result = page(parent, "Settings", back, context);
    section(result.content, "Wireless");
    row(result.content, LV_SYMBOL_WIFI, "Wi-Fi", wifi, selected, context, 1);
    section(result.content, "Media & display");
    row(result.content, LV_SYMBOL_VOLUME_MAX, "Sound", "Speaker volume", selected, context, 2);
    row(result.content, LV_SYMBOL_IMAGE, "Display", "Backlight & screen information", selected, context, 3);
    section(result.content, "System");
    row(result.content, LV_SYMBOL_SETTINGS, "About", "Device & firmware information", selected, context, 4);
    return result;
}

ControlPage display(lv_obj_t *parent, int brightness, lv_event_cb_t back, lv_event_cb_t changed, void *context)
{
    ControlPage result;
    result.page = page(parent, "Display", back, context);
    result.control = slider(result.page.content, "Brightness", "Adjust the screen backlight", brightness, 20, 100, changed, context);
    section(result.page.content, "Screen");
    row(result.page.content, LV_SYMBOL_IMAGE, "Resolution", "800 x 1280");
    row(result.page.content, LV_SYMBOL_REFRESH, "Orientation", "Portrait");
    return result;
}

ControlPage sound(lv_obj_t *parent, int volume, lv_event_cb_t back, lv_event_cb_t changed, void *context)
{
    ControlPage result;
    result.page = page(parent, "Sound", back, context);
    result.control = slider(result.page.content, "Volume", "Adjust the speaker output level", volume, 0, 95, changed, context);
    section(result.page.content, "Playback");
    row(result.page.content, LV_SYMBOL_AUDIO, "Audio output", "Speaker");
    note(result.page.content, "Volume changes apply to music and video playback.");
    return result;
}

Page about(lv_obj_t *parent, const char *version, lv_event_cb_t back, void *context)
{
    auto result = page(parent, "About", back, context);
    section(result.content, "Device");
    row(result.content, LV_SYMBOL_HOME, "Manufacturer", "Waveshare");
    row(result.content, LV_SYMBOL_SETTINGS, "Platform", "ESP32-P4");
    row(result.content, LV_SYMBOL_IMAGE, "Display", "800 x 1280 / Portrait");
    section(result.content, "Software");
    row(result.content, LV_SYMBOL_DIRECTORY, "ESP-IDF", version);
    row(result.content, LV_SYMBOL_LIST, "Interface", "ESP-Brookesia / LVGL");
    return result;
}

Page PlayerView::create(lv_obj_t *parent, const void *art, lv_event_cb_t back, lv_event_cb_t action, void *context)
{
    auto result = page(parent, "Music", back, context);
    auto *cover = card(result.content, 480);
    auto *disc = plain(cover);
    lv_obj_set_size(disc, 344, 344);
    lv_obj_center(disc);
    lv_obj_set_style_radius(disc, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(disc, lv_color_hex(0x111B2A), 0);
    lv_obj_set_style_bg_opa(disc, LV_OPA_COVER, 0);
    for (int size : {300, 248, 196}) {
        auto *groove = plain(disc);
        lv_obj_set_size(groove, size, size);
        lv_obj_center(groove);
        lv_obj_set_style_radius(groove, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(groove, 2, 0);
        lv_obj_set_style_border_color(groove, lv_color_hex(0x25374D), 0);
    }
    image(disc, art, 112);
    _title = label(result.content, "", &lv_font_montserrat_40);
    lv_obj_set_width(_title, lv_pct(100));
    lv_label_set_long_mode(_title, LV_LABEL_LONG_DOT);
    _track = label(result.content, "On-device music", &lv_font_montserrat_24, HINT);
    _state = label(result.content, "Ready to play", &lv_font_montserrat_24, HINT);
    _elapsed = label(result.content, "00:00 elapsed", &lv_font_montserrat_24, HINT);
    auto *controls = plain(result.content);
    lv_obj_set_size(controls, lv_pct(100), 112);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    const char *symbols[] = {LV_SYMBOL_PREV, LV_SYMBOL_PLAY, LV_SYMBOL_NEXT, LV_SYMBOL_LIST};
    for (intptr_t i = 0; i < 4; ++i) {
        auto *button = card(controls, i == 1 ? 104 : 88);
        lv_obj_set_width(button, i == 1 ? 104 : 88);
        if (i == 1) lv_obj_set_style_bg_color(button, lv_color_hex(ACCENT), 0);
        pressable(button);
        lv_obj_set_user_data(button, reinterpret_cast<void *>(i));
        lv_obj_add_event_cb(button, action, LV_EVENT_CLICKED, context);
        auto *icon = label(button, symbols[i], &lv_font_montserrat_36);
        lv_obj_center(icon);
        if (i == 1) _play = icon;
    }
    return result;
}

void PlayerView::title(const char *name, int index, int count)
{
    lv_label_set_text(_title, name);
    lv_label_set_text_fmt(_track, "Track %d of %d  /  On-device music", index + 1, count);
}
void PlayerView::playing(bool active)
{
    lv_label_set_text(_play, active ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    lv_label_set_text(_state, active ? "Playing" : "Paused");
}
void PlayerView::message(const char *text) { lv_label_set_text(_state, text); }
void PlayerView::elapsed(uint32_t seconds)
{
    lv_label_set_text_fmt(_elapsed, "%02u:%02u elapsed", static_cast<unsigned>(seconds / 60), static_cast<unsigned>(seconds % 60));
}
void PlayerView::clear() { _title = _track = _state = _elapsed = _play = nullptr; }
} // namespace factory_ui
