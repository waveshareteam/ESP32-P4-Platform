/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "lvgl.h"
#include "esp_brookesia.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Drawpanel"
#include "esp_lib_utils.h"
#include "Drawpanel.hpp"
#include <list>

#define APP_NAME "DrawPanel"

using namespace std;
using namespace esp_brookesia::gui;
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
    App(APP_NAME, &img_app_drawpanel, true, use_status_bar, use_navigation_bar),
    _max_points(1000)
{
}

Drawpanel::~Drawpanel()
{
    clearAllPoints();
}

bool Drawpanel::run(void)
{
    ESP_UTILS_LOGD("Run");
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFE0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);

    lv_area_t area = getVisualArea();
    int width = lv_area_get_width(&area);
    int height = lv_area_get_height(&area);

    _panel_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_panel_obj, width, height);
    lv_obj_set_pos(_panel_obj, area.x1, area.y1);

    lv_obj_add_event_cb(_panel_obj, touch_event_cb, LV_EVENT_PRESSING, this);

    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_VER);

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
    
    clearAllPoints();

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

    return true;
}

bool Drawpanel::resume()
{
    ESP_UTILS_LOGD("Resume");

    return true;
}

void Drawpanel::clearAllPoints()
{
    for (lv_obj_t* dot : _dots) {
        lv_obj_del(dot);
    }
    _dots.clear();
}

void Drawpanel::touch_event_cb(lv_event_t *e)
{
    Drawpanel *app = (Drawpanel *)lv_event_get_user_data(e);
    if (!app) {
        ESP_UTILS_LOGE("Get drawpanel instance failed");
        return;
    }

    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);

    lv_area_t area = app->getVisualArea();
    if (point.x < area.x1 || point.x > area.x2 || point.y < area.y1 || point.y > area.y2) {
        return;
    }
    
    lv_obj_t *dot = lv_obj_create(app->_panel_obj ? app->_panel_obj : lv_scr_act());
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_pos(dot, point.x - area.x1, point.y - area.y1);
    lv_obj_set_style_bg_color(dot, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_radius(dot, 5, 0);
    
    app->_dots.push_back(dot);
    
    if (app->_dots.size() > app->_max_points) {
        lv_obj_t* oldest_dot = app->_dots.front();
        lv_obj_del(oldest_dot);
        app->_dots.pop_front();
    }
}

} // namespace esp_brookesia::apps
