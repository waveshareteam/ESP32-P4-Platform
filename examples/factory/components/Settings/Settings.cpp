/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "factory_phone.hpp"
#include "Settings.hpp"
#include "factory_ui.hpp"
#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "esp_app_desc.h"
#include <cstdio>
#include <stdlib.h>
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Settings"
#include "esp_lib_utils.h"

#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TIMEOUT_MS 1000
// Settings page icons.
LV_IMG_DECLARE(WIFI);
LV_IMG_DECLARE(sound);
LV_IMG_DECLARE(info);
LV_IMG_DECLARE(BLE);
LV_IMG_DECLARE(backlights);
LV_IMG_DECLARE(arrow);
LV_IMG_DECLARE(img_app_settings);

namespace esp_brookesia::apps
{
    Settings *Settings::_instance = nullptr;

    Settings *Settings::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new Settings(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    Settings::Settings(bool use_status_bar, bool use_navigation_bar) : App(factory_ui::screen_config("Settings", &img_app_settings), factory_ui::app_config(use_status_bar, use_navigation_bar)),
                                                                       list1(nullptr),
                                                                       active_page(ActivePage::None)
    {
    }

    Settings::~Settings()
    {
    }

    void Settings::create_settings_ui()
    {
        lv_obj_clean(lv_scr_act());
        const auto wifi = factory_wifi::snapshot();
        const char *network = wifi.ip[0] ? wifi.ssid : wifi.enabled ? "Not connected" : "Off";
        char volume[8];
        char brightness[8];
        char firmware[48];
        snprintf(volume, sizeof(volume), "%d%%", product_codec_volume_get());
        snprintf(brightness, sizeof(brightness), "%d%%", product_display_brightness_get());
        snprintf(firmware, sizeof(firmware), "Version %s", esp_app_get_description()->version);
        auto view = factory_ui::page(lv_scr_act(), "Settings", nullptr, this);
        factory_ui::row(view.content, &WIFI, "Wi-Fi", network, event_handler_cb, this, 1);
        auto *levels = factory_ui::plain(view.content);
        lv_obj_set_size(levels, lv_pct(100), 142);
        lv_obj_set_flex_flow(levels, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(levels, 16, 0);
        auto add_level = [&](const void *icon, const char *name, const char *value, intptr_t id) {
            auto tile = factory_ui::row(levels, icon, name, value, event_handler_cb, this, id);
            lv_obj_set_style_pad_all(tile.object, 18, 0);
            lv_obj_set_style_pad_column(tile.object, 12, 0);
            lv_obj_set_width(tile.object, 0);
            lv_obj_set_height(tile.object, lv_pct(100));
            lv_obj_set_flex_grow(tile.object, 1);
        };
        add_level(&sound, "Sound", volume, 2);
        add_level(&backlights, "Display", brightness, 3);
        factory_ui::row(view.content, &info, "About", firmware, event_handler_cb, this, 4);
        list1 = view.root;
    }

    void Settings::destroy_settings_ui()
    {
        if (list1) {
            lv_obj_delete(list1);
            list1 = nullptr;
        }
    }

    void Settings::event_handler_cb(lv_event_t *e)
    {
        Settings *instance = static_cast<Settings *>(lv_event_get_user_data(e));
        if (instance)
        {
            instance->event_handler(e);
        }
    }

    void Settings::open_page_async_cb(void *user_data)
    {
        const char *page_name = static_cast<const char *>(user_data);
        Settings *instance = Settings::requestInstance(true, false);
        if (instance && instance->list1 && page_name)
        {
            instance->open_page(page_name);
        }
    }

    void Settings::event_handler(lv_event_t *e)
    {
        auto *object = static_cast<lv_obj_t *>(lv_event_get_current_target(e));
        const intptr_t choice = reinterpret_cast<intptr_t>(lv_obj_get_user_data(object));
        const char *pages[] = {"WLAN", "Sound", "Display", "About"};
        if (choice >= 1 && choice <= 4) {
            lv_async_call(open_page_async_cb, const_cast<char *>(pages[choice - 1]));
        }
    }

    void Settings::open_page(const char *page_name)
    {
        destroy_settings_ui();

        if (strcmp(page_name, "WLAN") == 0)
        {
            active_page = ActivePage::Wlan;
            wlan.open(lv_screen_active(), [](void *context) {
                auto *settings = static_cast<Settings *>(context);
                if (settings->active_page == ActivePage::Wlan) {
                    settings->wlan.close();
                    settings->run();
                }
            }, this);
        }
        else if (strcmp(page_name, "Sound") == 0)
        {
            active_page = ActivePage::Sound;
            auto sound = SoundPage::requestInstance(true, false);
            sound->run();
        }
        else if (strcmp(page_name, "Display") == 0)
        {
            active_page = ActivePage::Display;
            auto display = DisplayPage::requestInstance(true, false);
            display->run();
        }
        else if (strcmp(page_name, "About") == 0)
        {
            active_page = ActivePage::About;
            auto about = AboutPage::requestInstance(true, false);
            about->run();
        }
    }

    int Settings::pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
    {
        Settings *instance = Settings::requestInstance();
        esp_err_t ret = i2c_master_transmit_receive(instance->pmu_dev_handle, &regAddr, 1, data, len, I2C_MASTER_TIMEOUT_MS);
        if (ret != ESP_OK)
        {
            ESP_UTILS_LOGI("PMU READ FAILED!");
            return -1;
        }
        return 0;
    }

    int Settings::pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
    {
        Settings *instance = Settings::requestInstance();
        uint8_t *buffer = (uint8_t *)malloc(len + 1);
        if (!buffer)
            return -1;
        buffer[0] = regAddr;
        memcpy(&buffer[1], data, len);

        esp_err_t ret = i2c_master_transmit(instance->pmu_dev_handle, buffer, len + 1, I2C_MASTER_TIMEOUT_MS);
        free(buffer);

        if (ret != ESP_OK)
        {
            ESP_UTILS_LOGI("PMU WRITE FAILED!");
            return -1;
        }
        return 0;
    }

    bool Settings::run(void)
    {
        ESP_UTILS_LOGD("Run");
        active_page = ActivePage::None;
        if (list1 == nullptr)
        {
            create_settings_ui();
        }
        return true;
    }

    bool Settings::back(void)
    {
        ESP_UTILS_LOGD("Back");
        return notifyCoreClosed();
    }

    bool Settings::close(void)
    {
        ESP_UTILS_LOGD("Close");
        switch (active_page)
        {
        case ActivePage::Wlan:
            wlan.close();
            break;
        case ActivePage::Sound:
            SoundPage::requestInstance(true, false)->close();
            break;
        case ActivePage::Display:
            DisplayPage::requestInstance(true, false)->close();
            break;
        case ActivePage::About:
            AboutPage::requestInstance(true, false)->close();
            break;
        case ActivePage::None:
        default:
            break;
        }
        active_page = ActivePage::None;
        destroy_settings_ui();
        return true;
    }

    bool Settings::init()
    {
        ESP_UTILS_LOGD("Init");
        return true;
    }

    bool Settings::deinit()
    {
        ESP_UTILS_LOGD("Deinit");
        return true;
    }

    bool Settings::pause()
    {
        ESP_UTILS_LOGD("Pause");
        return true;
    }

    bool Settings::resume()
    {
        ESP_UTILS_LOGD("Resume");
        return true;
    }

} // namespace esp_brookesia::apps
