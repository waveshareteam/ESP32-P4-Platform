/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "SoundPage.hpp"
#include "../Settings.hpp"
#include "factory_ui.hpp"
#include "product_support.h"
#include "bsp/esp-bsp.h"
#include <cstdio>

namespace esp_brookesia::apps {
SoundPage *SoundPage::_instance = nullptr;
SoundPage *SoundPage::requestInstance(bool status, bool navigation)
{
    if (!_instance) _instance = new SoundPage(status, navigation);
    return _instance;
}
SoundPage::SoundPage(bool status, bool navigation) : App("Sound", nullptr, true, status, navigation), label(nullptr), list1(nullptr) {}
SoundPage::~SoundPage() {}
bool SoundPage::run()
{
    lv_obj_clean(lv_scr_act());
    auto view = factory_ui::sound(lv_scr_act(), product_codec_volume_get(), [](lv_event_t *) {
        lv_async_call([](void *) {
            SoundPage::requestInstance(true, false)->close();
            Settings::requestInstance(true, false)->run();
        }, nullptr);
    }, event_handler_cb, this);
    list1 = view.page.root;
    label = view.control.value;
    return true;
}
void SoundPage::event_handler_cb(lv_event_t *event)
{
    auto *self = static_cast<SoundPage *>(lv_event_get_user_data(event));
    auto *control = static_cast<lv_obj_t *>(lv_event_get_current_target(event));
    const int value = lv_arc_get_value(control);
    product_codec_volume_set(value, nullptr);
    lv_label_set_text_fmt(self->label, "%d%%", value);
}
bool SoundPage::back() { return notifyCoreClosed(); }
bool SoundPage::close()
{
    if (list1) lv_obj_delete(list1);
    list1 = label = nullptr;
    return true;
}
} // namespace esp_brookesia::apps
