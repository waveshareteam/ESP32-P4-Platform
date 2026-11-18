/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Drawpanel.hpp"
#include "factory_phone.hpp"
#include "factory_ui.hpp"
#include "lvgl.h"
#include <algorithm>
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Drawpanel"
#include "esp_lib_utils.h"

#define APP_NAME "Draw"

using namespace esp_brookesia::systems;

LV_IMG_DECLARE(img_app_drawpanel);

namespace esp_brookesia::apps {

Drawpanel *Drawpanel::_instance = nullptr;

Drawpanel *Drawpanel::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new Drawpanel(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

Drawpanel::Drawpanel(bool use_status_bar, bool use_navigation_bar) :
    App(factory_ui::screen_config(APP_NAME, &img_app_drawpanel),
        factory_ui::app_config(use_status_bar, use_navigation_bar)),
    _pen_color(lv_color_black())
{
}

Drawpanel::~Drawpanel()
{
    if (_draw_buf) {
        lv_draw_buf_destroy(_draw_buf);
    }
}

bool Drawpanel::run(void)
{
    ESP_UTILS_LOGD("Run");

    lv_area_t area = getVisualArea();
    if (getActiveConfig().status_bar_visual_mode == phone::StatusBar::VisualMode::SHOW_FIXED &&
            area.y1 < factory_ui::STATUS_HEIGHT) {
        area.y1 = factory_ui::STATUS_HEIGHT;
    }
    const int width = lv_area_get_width(&area);
    const int height = lv_area_get_height(&area);
    ESP_UTILS_CHECK_FALSE_RETURN(width > 0 && height > 0, false, "Invalid canvas geometry");

    if (!_draw_buf) {
        _draw_buf = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
        ESP_UTILS_CHECK_NULL_RETURN(_draw_buf, false, "Create canvas buffer failed");
    }

    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(factory_ui::BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    _canvas = lv_canvas_create(screen);
    lv_canvas_set_draw_buf(_canvas, _draw_buf);
    lv_obj_set_pos(_canvas, area.x1, area.y1);
    lv_obj_set_style_border_width(_canvas, 0, 0);
    lv_obj_set_style_pad_all(_canvas, 0, 0);
    lv_obj_remove_flag(_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_canvas, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(_canvas, canvas_event_cb, LV_EVENT_ALL, this);
    clearCanvas();

    createToolbar(width);

    return true;
}

bool Drawpanel::back(void)
{
    ESP_UTILS_LOGD("Back");

    // If the app needs to exit, call notifyCoreClosed() to notify the core to close the app
    ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");

    return true;
}

bool Drawpanel::close(void)
{
    ESP_UTILS_LOGD("Close");

    if (_canvas) {
        lv_obj_delete(_canvas);
        _canvas = nullptr;
    }
    if (_draw_buf) {
        lv_draw_buf_destroy(_draw_buf);
        _draw_buf = nullptr;
    }
    _selected_swatch = nullptr;
    _stroke_active = false;

    return true;
}

bool Drawpanel::init()
{
    ESP_UTILS_LOGD("Init");

    return true;
}

bool Drawpanel::deinit()
{
    ESP_UTILS_LOGD("Deinit");

    return true;
}

bool Drawpanel::pause()
{
    ESP_UTILS_LOGD("Pause");
    _stroke_active = false;

    return true;
}

bool Drawpanel::resume()
{
    ESP_UTILS_LOGD("Resume");

    return true;
}

void Drawpanel::createToolbar(int canvas_width)
{
    static constexpr uint32_t colors[] = {0x000000, 0xE53935, 0xFB8C00, 0x43A047, 0x1E88E5, 0x8E24AA};
    constexpr int reference_width = 466;
    const int swatch_size = canvas_width * 34 / reference_width;
    const int padding = canvas_width * 8 / reference_width;
    const int gap = canvas_width * 10 / reference_width;
    const int bottom_gap = canvas_width * 16 / reference_width;

    lv_obj_t *bar = lv_obj_create(lv_obj_get_parent(_canvas));
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_90, 0);
    lv_obj_set_style_radius(bar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(bar, padding, 0);
    lv_obj_set_style_pad_column(bar, gap, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    for (uint32_t color : colors) {
        lv_obj_t *swatch = factory_ui::plain(bar);
        lv_obj_set_size(swatch, swatch_size, swatch_size);
        lv_obj_set_style_radius(swatch, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(swatch, lv_color_hex(color), 0);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, 0);
        lv_obj_set_style_outline_color(swatch, lv_color_hex(0x9E9E9E), 0);
        lv_obj_set_style_outline_pad(swatch, 2, 0);
        lv_obj_set_style_outline_width(swatch, 3, LV_STATE_CHECKED);
        lv_obj_add_flag(swatch, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(swatch, swatch_event_cb, LV_EVENT_CLICKED, this);
        if (lv_color_eq(lv_color_hex(color), _pen_color)) {
            lv_obj_add_state(swatch, LV_STATE_CHECKED);
            _selected_swatch = swatch;
        }
    }

    lv_obj_t *clear = lv_button_create(bar);
    lv_obj_set_size(clear, swatch_size, swatch_size);
    lv_obj_set_style_radius(clear, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(clear, lv_color_white(), 0);
    lv_obj_set_style_shadow_width(clear, 0, 0);
    lv_obj_add_event_cb(clear, clear_event_cb, LV_EVENT_CLICKED, this);
    lv_obj_t *icon = factory_ui::label(clear, LV_SYMBOL_TRASH, &lv_font_montserrat_28, 0x424242);
    lv_obj_center(icon);

    lv_obj_align_to(bar, _canvas, LV_ALIGN_BOTTOM_MID, 0, -factory_ui::GESTURE_INSET - bottom_gap);
}

void Drawpanel::clearCanvas(void)
{
    if (_canvas) {
        lv_canvas_fill_bg(_canvas, lv_color_white(), LV_OPA_COVER);
    }
}

void Drawpanel::drawSegment(const lv_point_t &from, const lv_point_t &to)
{
    constexpr int32_t stroke_width = 8;
    lv_layer_t layer;
    lv_canvas_init_layer(_canvas, &layer);

    lv_area_t dirty;
    if (from.x == to.x && from.y == to.y) {
        // The software renderer skips lines whose endpoints coincide.
        constexpr int32_t radius = stroke_width / 2;
        dirty = {from.x - radius, from.y - radius, from.x + radius - 1, from.y + radius - 1};
        lv_draw_fill_dsc_t dot;
        lv_draw_fill_dsc_init(&dot);
        dot.color = _pen_color;
        dot.radius = LV_RADIUS_CIRCLE;
        lv_draw_fill(&layer, &dot, &dirty);
    } else {
        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.color = _pen_color;
        line.width = stroke_width;
        line.round_start = 1;
        line.round_end = 1;
        line.p1.x = from.x;
        line.p1.y = from.y;
        line.p2.x = to.x;
        line.p2.y = to.y;
        lv_draw_line(&layer, &line);

        // Match LVGL's line bounds to include antialiasing and rounded ends.
        dirty = {std::min(from.x, to.x) - stroke_width, std::min(from.y, to.y) - stroke_width,
                 std::max(from.x, to.x) + stroke_width, std::max(from.y, to.y) + stroke_width};
    }

    // Finish rasterizing without the canvas helper invalidating its entire image.
    lv_display_t *display = lv_obj_get_display(_canvas);
    lv_display_enable_invalidation(display, false);
    lv_canvas_finish_layer(_canvas, &layer);
    lv_display_enable_invalidation(display, true);

    lv_area_t canvas_coords;
    lv_obj_get_coords(_canvas, &canvas_coords);
    lv_area_move(&dirty, canvas_coords.x1, canvas_coords.y1);
    dirty.x1 = std::max(dirty.x1, canvas_coords.x1);
    dirty.y1 = std::max(dirty.y1, canvas_coords.y1);
    dirty.x2 = std::min(dirty.x2, canvas_coords.x2);
    dirty.y2 = std::min(dirty.y2, canvas_coords.y2);
    lv_obj_invalidate_area(_canvas, &dirty);
}

void Drawpanel::canvas_event_cb(lv_event_t *e)
{
    Drawpanel *app = (Drawpanel *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (!app || !app->_canvas) {
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        app->_stroke_active = false;
        return;
    }
    if (code != LV_EVENT_PRESSED && (code != LV_EVENT_PRESSING || !app->_stroke_active)) {
        return;
    }

    lv_indev_t *indev = lv_indev_active();
    if (!indev) {
        return;
    }
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    lv_area_t coords;
    lv_obj_get_coords(app->_canvas, &coords);
    if (point.x < coords.x1 || point.x > coords.x2 || point.y < coords.y1 || point.y > coords.y2) {
        app->_stroke_active = false;
        return;
    }
    point.x -= coords.x1;
    point.y -= coords.y1;

    if (code == LV_EVENT_PRESSED) {
        // Reserve bottom-edge presses for the system's swipe-up gesture.
        app->_stroke_active = lv_area_get_height(&coords) - point.y >= factory_ui::GESTURE_INSET;
        if (!app->_stroke_active) {
            return;
        }
        app->drawSegment(point, point);
    } else if (point.x != app->_last_point.x || point.y != app->_last_point.y) {
        app->drawSegment(app->_last_point, point);
    }
    app->_last_point = point;
}

void Drawpanel::swatch_event_cb(lv_event_t *e)
{
    Drawpanel *app = (Drawpanel *)lv_event_get_user_data(e);
    lv_obj_t *swatch = (lv_obj_t *)lv_event_get_target(e);
    if (!app || !swatch) {
        return;
    }
    if (app->_selected_swatch) {
        lv_obj_remove_state(app->_selected_swatch, LV_STATE_CHECKED);
    }
    lv_obj_add_state(swatch, LV_STATE_CHECKED);
    app->_selected_swatch = swatch;
    app->_pen_color = lv_obj_get_style_bg_color(swatch, LV_PART_MAIN);
}

void Drawpanel::clear_event_cb(lv_event_t *e)
{
    Drawpanel *app = (Drawpanel *)lv_event_get_user_data(e);
    if (app) {
        app->clearCanvas();
    }
}

} // namespace esp_brookesia::apps
