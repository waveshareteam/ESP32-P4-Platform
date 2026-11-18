/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "WlanPage.hpp"
#include "esp_log.h"
#include <cstring>

LV_IMAGE_DECLARE(WIFI);
LV_IMAGE_DECLARE(wifi_1);
LV_IMAGE_DECLARE(wifi_2);
LV_IMAGE_DECLARE(wifi_3);
LV_IMAGE_DECLARE(wifi_4);

namespace esp_brookesia::apps {
namespace {
constexpr char TAG[] = "FactoryWiFiUI";
using factory_wifi::State;

bool operation_in_progress(State state)
{
    return state == State::Starting || state == State::Scanning || state == State::Connecting;
}

const char *status_text(const factory_wifi::Snapshot &wifi)
{
    if (wifi.message[0]) return wifi.message;
    switch (wifi.state) {
    case State::Off: return "Wi-Fi is off";
    case State::Starting: return "Starting Wi-Fi…";
    case State::Scanning: return "Looking for networks…";
    case State::Connecting: return "Joining network…";
    case State::Connected: return "Connected";
    case State::Error: return "Wi-Fi unavailable. Try again.";
    case State::Idle: return "Choose a network to join";
    }
    return "";
}
} // namespace

void WlanPage::open(lv_obj_t *parent, void (*leave)(void *), void *context)
{
    _leave = leave;
    _context = context;
    _first_refresh = true;
    _page = factory_ui::page(parent, "Wi-Fi", [](lv_event_t *event) {
        auto *self = static_cast<WlanPage *>(lv_event_get_user_data(event));
        if (self->_editor) {
            self->hide_password();
        } else {
            lv_async_call(self->_leave, self->_context);
        }
    }, this);
    auto toggle = factory_ui::row(_page.content, &WIFI, "Wi-Fi", nullptr);
    lv_obj_add_flag(toggle.value, LV_OBJ_FLAG_HIDDEN);
    _switch = lv_switch_create(toggle.object);
    lv_obj_set_size(_switch, 88, 48);
    lv_obj_set_style_bg_color(_switch, lv_color_hex(factory_ui::ACCENT),
        static_cast<lv_style_selector_t>(LV_PART_INDICATOR) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
    lv_obj_add_event_cb(_switch, [](lv_event_t *event) {
        auto *self = static_cast<WlanPage *>(lv_event_get_user_data(event));
        const bool enabled = lv_obj_has_state(self->_switch, LV_STATE_CHECKED);
        if (!factory_wifi::set_enabled(enabled)) {
            if (enabled) lv_obj_remove_state(self->_switch, LV_STATE_CHECKED);
            else lv_obj_add_state(self->_switch, LV_STATE_CHECKED);
            self->report_busy();
            return;
        }
        lv_obj_add_state(self->_switch, LV_STATE_DISABLED);
        lv_label_set_text(self->_status, enabled ? "Starting Wi-Fi…" : "Turning Wi-Fi off…");
    }, LV_EVENT_VALUE_CHANGED, this);
    _status = factory_ui::note(_page.content, "");
    _results = factory_ui::plain(_page.content);
    lv_obj_set_size(_results, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(_results, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(_results, 12, 0);
    refresh();
    _timer = lv_timer_create([](lv_timer_t *timer) {
        static_cast<WlanPage *>(lv_timer_get_user_data(timer))->refresh();
    }, 200, this);
    ESP_LOGI(TAG, "Wi-Fi page ready");
}

void WlanPage::close()
{
    if (_timer) lv_timer_delete(_timer);
    _timer = nullptr;
    if (_page.root) lv_obj_delete(_page.root);
    _page = {};
    _switch = _status = _results = _editor = _password = _feedback = nullptr;
    ESP_LOGI(TAG, "Wi-Fi page closed");
}

void WlanPage::refresh()
{
    auto latest = factory_wifi::snapshot();
    if (!_first_refresh && latest.revision == _snapshot.revision) return;
    _first_refresh = false;
    _snapshot = latest;
    if (latest.enabled) lv_obj_add_state(_switch, LV_STATE_CHECKED);
    else lv_obj_remove_state(_switch, LV_STATE_CHECKED);
    lv_obj_remove_state(_switch, LV_STATE_DISABLED);
    lv_label_set_text(_status, status_text(latest));
    lv_obj_clean(_results);
    if (!latest.enabled) return;

    if (latest.ip[0]) {
        factory_ui::row(_results, LV_SYMBOL_OK, latest.ssid, latest.ip);
        factory_ui::row(_results, LV_SYMBOL_TRASH, "Forget network", nullptr, action_cb, this, FORGET);
    }
    auto scan = factory_ui::row(_results, LV_SYMBOL_REFRESH, "Scan for networks", nullptr, action_cb, this, SCAN);
    lv_obj_add_flag(scan.value, LV_OBJ_FLAG_HIDDEN);
    if (operation_in_progress(latest.state)) lv_obj_add_state(scan.object, LV_STATE_DISABLED);
    if (latest.count) factory_ui::section(_results, "Available networks");
    for (int i = 0; i < latest.count; ++i) {
        const auto &network = latest.networks[i];
        const auto *icon = network.rssi >= -55 ? &wifi_4 : network.rssi >= -67 ? &wifi_3 :
            network.rssi >= -78 ? &wifi_2 : &wifi_1;
        auto item = factory_ui::row(_results, icon, network.ssid,
                                    network.secured ? "Secured" : "Open", action_cb, this, i);
        if (operation_in_progress(latest.state)) lv_obj_add_state(item.object, LV_STATE_DISABLED);
    }
}

void WlanPage::report_busy()
{
    lv_label_set_text(_status, "Wi-Fi is busy. Try again in a moment.");
}

void WlanPage::action_cb(lv_event_t *event)
{
    auto *self = static_cast<WlanPage *>(lv_event_get_user_data(event));
    auto *object = lv_event_get_current_target_obj(event);
    const auto action = reinterpret_cast<intptr_t>(lv_obj_get_user_data(object));
    if (action == SCAN || action == FORGET) {
        const bool queued = action == SCAN ? factory_wifi::scan() : factory_wifi::forget();
        if (queued) {
            lv_obj_add_state(object, LV_STATE_DISABLED);
            lv_label_set_text(self->_status, action == SCAN ? "Looking for networks…" : "Forgetting network…");
        } else {
            self->report_busy();
        }
        return;
    }
    if (action < 0 || action >= self->_snapshot.count) return;
    self->_selected = self->_snapshot.networks[action];
    if (self->_selected.secured) self->show_password(self->_selected);
    else self->send_join("");
}

void WlanPage::show_password(const factory_wifi::Network &network)
{
    lv_obj_add_flag(_page.content, LV_OBJ_FLAG_HIDDEN);
    _editor = factory_ui::plain(_page.root);
    lv_obj_set_size(_editor, factory_ui::WIDTH - 2 * factory_ui::GUTTER,
        factory_ui::HEIGHT - factory_ui::CONTENT_TOP - factory_ui::GESTURE_INSET - 20);
    lv_obj_set_pos(_editor, factory_ui::GUTTER, factory_ui::CONTENT_TOP);
    lv_obj_set_flex_flow(_editor, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(_editor, 16, 0);
    auto *title = factory_ui::label(_editor, network.ssid, &lv_font_montserrat_36);
    lv_obj_set_width(title, lv_pct(100));
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    _password = lv_textarea_create(_editor);
    lv_obj_set_size(_password, lv_pct(100), 78);
    lv_obj_set_style_bg_color(_password, lv_color_hex(factory_ui::CARD), 0);
    lv_obj_set_style_text_color(_password, lv_color_white(), 0);
    lv_obj_set_style_text_font(_password, &lv_font_montserrat_28, 0);
    lv_textarea_set_one_line(_password, true);
    lv_textarea_set_password_mode(_password, true);
    lv_textarea_set_max_length(_password, 63);
    lv_textarea_set_placeholder_text(_password, "Network password");
    _feedback = factory_ui::note(_editor, "Use 8–63 characters");
    auto *keyboard = lv_keyboard_create(_editor);
    lv_obj_set_size(keyboard, lv_pct(100), 420);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(factory_ui::CARD), 0);
    lv_obj_set_style_text_font(keyboard, &lv_font_montserrat_24, LV_PART_ITEMS);
    lv_keyboard_set_textarea(keyboard, _password);
    lv_obj_add_event_cb(keyboard, keyboard_cb, LV_EVENT_READY, this);
    lv_obj_add_event_cb(keyboard, keyboard_cb, LV_EVENT_CANCEL, this);
}

void WlanPage::hide_password()
{
    if (!_editor) return;
    lv_obj_delete_async(_editor);
    lv_obj_add_flag(_editor, LV_OBJ_FLAG_HIDDEN);
    _editor = _password = _feedback = nullptr;
    lv_obj_remove_flag(_page.content, LV_OBJ_FLAG_HIDDEN);
}

void WlanPage::send_join(const char *password)
{
    if (!factory_wifi::connect(_selected, password)) {
        if (_feedback) lv_label_set_text(_feedback, "Wi-Fi is busy. Try again in a moment.");
        else report_busy();
        return;
    }
    hide_password();
    lv_label_set_text(_status, "Joining network…");
}

void WlanPage::keyboard_cb(lv_event_t *event)
{
    auto *self = static_cast<WlanPage *>(lv_event_get_user_data(event));
    if (!self->_password) return;
    if (lv_event_get_code(event) == LV_EVENT_CANCEL) {
        self->hide_password();
        return;
    }
    const char *password = lv_textarea_get_text(self->_password);
    const size_t length = strlen(password);
    if (length < 8 || length > 63) {
        lv_label_set_text(self->_feedback, "Use 8–63 characters");
        return;
    }
    self->send_join(password);
}

} // namespace esp_brookesia::apps
