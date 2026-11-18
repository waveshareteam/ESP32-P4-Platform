/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "AboutPage.hpp"
#include "../Settings.hpp"
#include "factory_ui.hpp"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_system.h"
#include <cstdio>

LV_IMAGE_DECLARE(logo);

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
    auto view = factory_ui::page(lv_scr_act(), "About", [](lv_event_t *) {
        lv_async_call([](void *) {
            AboutPage::requestInstance(true, false)->close();
            Settings::requestInstance(true, false)->run();
        }, nullptr);
    }, this);
    list1 = view.root;
    auto *column = view.content;
    lv_obj_set_flex_align(column, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    auto *emblem = lv_image_create(column);
    lv_image_set_src(emblem, &logo);
    lv_obj_set_size(emblem, 112, 112);
    lv_image_set_inner_align(emblem, LV_IMAGE_ALIGN_CONTAIN);
    lv_obj_set_style_margin_top(emblem, 24, 0);
    factory_ui::label(column, "ESP32-P4-Platform", &lv_font_montserrat_36);
    auto *maker = factory_ui::label(column, "Waveshare", &lv_font_montserrat_24, factory_ui::HINT);
    lv_obj_set_style_margin_bottom(maker, 16, 0);

    auto add_item = [column](const char *heading, const char *text) {
        auto *title = factory_ui::label(column, heading, &lv_font_montserrat_22, factory_ui::HINT);
        lv_obj_set_style_margin_top(title, 12, 0);
        auto *value = factory_ui::label(column, text, &lv_font_montserrat_28);
        lv_obj_set_width(value, lv_pct(100));
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, 0);
    };
    char text[112];
    const auto *app = esp_app_get_description();
    snprintf(text, sizeof(text), "%s, %s", app->version, app->date);
    add_item("Firmware", text);
    snprintf(text, sizeof(text), "ESP-IDF %s, ESP-Brookesia", esp_get_idf_version());
    add_item("Software", text);
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash = 0;
    esp_flash_get_size(nullptr, &flash);
    snprintf(text, sizeof(text), "ESP32-P4 v%d.%d, flash %u MB, PSRAM %u MB",
             chip.revision / 100, chip.revision % 100, static_cast<unsigned>(flash >> 20),
             static_cast<unsigned>(esp_psram_get_size() >> 20));
    add_item("Hardware", text);
    snprintf(text, sizeof(text), "%d x %d", factory_ui::WIDTH, factory_ui::HEIGHT);
    add_item("Display", text);
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
