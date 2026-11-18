/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "DisplayPage.hpp"
#include "../Settings.hpp"
#include "factory_ui.hpp"
#include "product_support.h"
#include "bsp/esp-bsp.h"
#include <cstdio>

namespace esp_brookesia::apps {
DisplayPage *DisplayPage::_instance = nullptr;
DisplayPage *DisplayPage::requestInstance(bool status, bool navigation)
{
    if (!_instance) _instance = new DisplayPage(status, navigation);
    return _instance;
}
DisplayPage::DisplayPage(bool status, bool navigation) : App("Display", nullptr, true, status, navigation), label(nullptr), list1(nullptr) {}
DisplayPage::~DisplayPage() {}
bool DisplayPage::run()
{
    lv_obj_clean(lv_scr_act());
    auto view = factory_ui::display(lv_scr_act(), product_display_brightness_get(), [](lv_event_t *) {
        lv_async_call([](void *) {
            DisplayPage::requestInstance(true, false)->close();
            Settings::requestInstance(true, false)->run();
        }, nullptr);
    }, event_handler_cb, this);
    list1 = view.page.root;
    label = view.control.value;
    return true;
}
void DisplayPage::event_handler_cb(lv_event_t *event)
{
    auto *self = static_cast<DisplayPage *>(lv_event_get_user_data(event));
    auto *control = static_cast<lv_obj_t *>(lv_event_get_current_target(event));
    const int value = lv_arc_get_value(control);
    bsp_display_brightness_set(value);
    lv_label_set_text_fmt(self->label, "%d%%", value);
}
bool DisplayPage::back() { return notifyCoreClosed(); }
bool DisplayPage::close()
{
    if (list1) lv_obj_delete(list1);
    list1 = label = nullptr;
    return true;
}
} // namespace esp_brookesia::apps
