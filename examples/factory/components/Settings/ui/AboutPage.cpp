/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "AboutPage.hpp"
#include "../Settings.hpp"
#include "factory_ui.hpp"
#include "esp_system.h"

namespace esp_brookesia::apps {
AboutPage *AboutPage::_instance = nullptr;
AboutPage *AboutPage::requestInstance(bool status, bool navigation)
{
    if (!_instance) _instance = new AboutPage(status, navigation);
    return _instance;
}
AboutPage::AboutPage(bool status, bool navigation) : App("About", nullptr, true, status, navigation), label(nullptr), list1(nullptr) {}
AboutPage::~AboutPage() {}
bool AboutPage::run()
{
    lv_obj_clean(lv_scr_act());
    auto view = factory_ui::about(lv_scr_act(), esp_get_idf_version(), [](lv_event_t *) {
        lv_async_call([](void *) {
            AboutPage::requestInstance(true, false)->close();
            Settings::requestInstance(true, false)->run();
        }, nullptr);
    }, this);
    list1 = view.root;
    return true;
}
bool AboutPage::back() { return notifyCoreClosed(); }
bool AboutPage::close()
{
    if (list1) lv_obj_delete(list1);
    list1 = label = nullptr;
    return true;
}
} // namespace esp_brookesia::apps
