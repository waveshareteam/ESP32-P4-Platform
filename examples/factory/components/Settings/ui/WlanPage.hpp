/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "factory_ui.hpp"
#include "wifi_service.hpp"

namespace esp_brookesia::apps {

class WlanPage {
public:
    void open(lv_obj_t *parent, void (*leave)(void *), void *context);
    void close();

private:
    enum Action : intptr_t { SCAN = -1, FORGET = -2 };
    factory_ui::Page _page;
    lv_timer_t *_timer = nullptr;
    lv_obj_t *_switch = nullptr;
    lv_obj_t *_status = nullptr;
    lv_obj_t *_results = nullptr;
    lv_obj_t *_editor = nullptr;
    lv_obj_t *_password = nullptr;
    lv_obj_t *_feedback = nullptr;
    factory_wifi::Snapshot _snapshot{};
    factory_wifi::Network _selected{};
    void (*_leave)(void *) = nullptr;
    void *_context = nullptr;
    bool _first_refresh = true;

    void refresh();
    void show_password(const factory_wifi::Network &network);
    void hide_password();
    void send_join(const char *password);
    void report_busy();
    static void action_cb(lv_event_t *event);
    static void keyboard_cb(lv_event_t *event);
};

} // namespace esp_brookesia::apps
