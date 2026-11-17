/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Settings.hpp"
#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
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
    static constexpr int SETTINGS_PAGE_MARGIN = 40;
    static constexpr int SETTINGS_LIST_WIDTH = BSP_LCD_H_RES - SETTINGS_PAGE_MARGIN * 2;
    static constexpr int SETTINGS_LIST_HEIGHT = BSP_LCD_V_RES - 220;
    static constexpr int SETTINGS_ROW_HEIGHT = 88;
    static bool s_time_sync_started = false;
    static bool s_time_synced = false;

    static void time_sync_notification_cb(struct timeval *tv)
    {
        (void)tv;
        setenv("TZ", "CST-8", 1);
        tzset();
        s_time_synced = true;
        ESP_UTILS_LOGI("SNTP time synced, timezone CST-8 applied");
    }

    static void start_time_sync_once()
    {
        if (s_time_sync_started || esp_sntp_enabled()) {
            return;
        }

        // Time sync is started lazily after Wi-Fi obtains an IP address, which avoids
        // SNTP retries before the network stack is ready.
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "ntp.aliyun.com");
        esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
        esp_sntp_init();
        s_time_sync_started = true;
        ESP_UTILS_LOGI("SNTP time sync started");
    }

    static void wifi_time_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        (void)arg;
        (void)event_data;

        if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            start_time_sync_once();
        }
    }

    Settings *Settings::_instance = nullptr;

    Settings *Settings::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new Settings(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    Settings::Settings(bool use_status_bar, bool use_navigation_bar) : App("Settings", &img_app_settings, true, use_status_bar, use_navigation_bar),
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
        list1 = nullptr;

        lv_style_init(&style_list);
        lv_style_init(&style_list_btn);
        lv_style_init(&style_list_text);
        lv_style_init(&style_list_btn_pressed);

        // List, row, and text styles are recreated whenever the settings root is shown.
        lv_style_set_pad_all(&style_list, 5);
        lv_style_set_pad_row(&style_list, 2);
        lv_style_set_border_width(&style_list, 0);
        lv_style_set_radius(&style_list, 10);
        lv_style_set_bg_color(&style_list, lv_color_hex(0x000000));
        lv_style_set_bg_opa(&style_list, LV_OPA_COVER);

        lv_style_set_height(&style_list_btn, SETTINGS_ROW_HEIGHT);
        lv_style_set_pad_all(&style_list_btn, 16);
        lv_style_set_border_width(&style_list_btn, 0);
        lv_style_set_radius(&style_list_btn, 8);
        lv_style_set_bg_color(&style_list_btn, lv_color_hex(0x38393A));
        lv_style_set_text_font(&style_list_btn, &lv_font_montserrat_30);
        lv_style_set_text_color(&style_list_btn, lv_color_hex(0xFFFFFF));
        lv_style_set_border_width(&style_list_btn, 1);
        lv_style_set_border_side(&style_list_btn, LV_BORDER_SIDE_BOTTOM);
        lv_style_set_border_color(&style_list_btn, lv_color_hex(0x505050));

        lv_style_set_text_font(&style_list_text, &lv_font_montserrat_30);
        lv_style_set_text_color(&style_list_text, lv_color_hex(0xFFFFFF));
        lv_style_set_bg_color(&style_list_text, lv_color_hex(0x000000));
        lv_style_set_bg_opa(&style_list_text, LV_OPA_COVER);
        lv_style_set_pad_all(&style_list_text, 10);
        lv_style_set_pad_bottom(&style_list_text, 5);

        lv_style_set_bg_color(&style_list_btn_pressed, lv_color_hex(0x505050));
        lv_style_set_text_color(&style_list_btn_pressed, lv_color_hex(0xFFFFFF));
        lv_style_set_border_color(&style_list_btn_pressed, lv_color_hex(0x505050));

        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);

        list1 = lv_list_create(lv_scr_act());
        lv_obj_add_style(list1, &style_list, 0);
        lv_obj_set_size(list1, SETTINGS_LIST_WIDTH, SETTINGS_LIST_HEIGHT);
        lv_obj_center(list1);
        lv_obj_set_scrollbar_mode(list1, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *text_obj = nullptr;
        text_obj = lv_list_add_text(list1, "Wireless");
        lv_obj_add_style(text_obj, &style_list_text, 0);
        add_button_with_arrow(&WIFI, "WLAN");
        // add_button_with_arrow(&BLE, "Bluetooth");

        text_obj = lv_list_add_text(list1, "Media");
        lv_obj_add_style(text_obj, &style_list_text, 0);
        add_button_with_arrow(&sound, "Sound");
        add_button_with_arrow(&backlights, "Display");

        text_obj = lv_list_add_text(list1, "More");
        lv_obj_add_style(text_obj, &style_list_text, 0);
        add_button_with_arrow(&info, "About");
    }

    void Settings::destroy_settings_ui()
    {
        if (list1)
        {
            lv_obj_del(list1);
            list1 = nullptr;
            lv_style_reset(&style_list);
            lv_style_reset(&style_list_btn);
            lv_style_reset(&style_list_text);
            lv_style_reset(&style_list_btn_pressed);
        }
    }

    void Settings::add_button_with_arrow(const void *icon, const char *text)
    {
        lv_obj_t *btn = lv_list_add_button(list1, icon, text);
        lv_obj_add_style(btn, &style_list_btn, 0);
        lv_obj_add_style(btn, &style_list_btn_pressed, LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn, event_handler_cb, LV_EVENT_CLICKED, this);

        lv_obj_t *arrow_img = lv_img_create(btn);
        lv_img_set_src(arrow_img, &arrow);
        lv_obj_align(arrow_img, LV_ALIGN_RIGHT_MID, -10, 0);
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
        if (instance && page_name)
        {
            instance->open_page(page_name);
        }
    }

    void Settings::event_handler(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED)
        {
            lv_obj_t *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
            const char *txt = lv_list_get_button_text(list1, obj);
            ESP_UTILS_LOGI("Clicked: %s", txt);

            if (strcmp(txt, "WLAN") == 0)
            {
                lv_async_call(open_page_async_cb, (void *)"WLAN");
            }
            else if (strcmp(txt, "Sound") == 0)
            {
                lv_async_call(open_page_async_cb, (void *)"Sound");
            }
            else if (strcmp(txt, "Display") == 0)
            {
                lv_async_call(open_page_async_cb, (void *)"Display");
            }
            else if (strcmp(txt, "About") == 0)
            {
                lv_async_call(open_page_async_cb, (void *)"About");
            }
        }
    }

    void Settings::open_page(const char *page_name)
    {
        destroy_settings_ui();

        if (strcmp(page_name, "WLAN") == 0)
        {
            active_page = ActivePage::Wlan;
            auto wlan = WlanPage::requestInstance(true, false);
            wlan->run();
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
            WlanPage::requestInstance(true, false)->close();
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
        initWifi();

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

    esp_err_t Settings::initWifi()
    {
        static bool netif_initialized = false;
        static bool event_loop_initialized = false;
        static bool wifi_initialized = false;
        static bool time_event_handler_registered = false;
        static esp_netif_t *sta_netif = nullptr;

        if (!netif_initialized) {
            esp_err_t ret = esp_netif_init();
            if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
                ESP_UTILS_LOGE("esp_netif_init failed: %s", esp_err_to_name(ret));
                return ret;
            }
            netif_initialized = true;
        }

        if (!event_loop_initialized) {
            esp_err_t ret = esp_event_loop_create_default();
            if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
                ESP_UTILS_LOGE("esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
                return ret;
            }
            event_loop_initialized = true;
        }

        if (!sta_netif) {
            sta_netif = esp_netif_create_default_wifi_sta();
            if (!sta_netif) {
                ESP_UTILS_LOGE("esp_netif_create_default_wifi_sta failed");
                return ESP_FAIL;
            }
        }

        if (!wifi_initialized) {
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            esp_err_t ret = esp_wifi_init(&cfg);
            if (ret != ESP_OK) {
                ESP_UTILS_LOGE("esp_wifi_init failed: %s", esp_err_to_name(ret));
                return ret;
            }
            wifi_initialized = true;
        }

        if (!time_event_handler_registered) {
            esp_err_t ret = esp_event_handler_register(
                IP_EVENT,
                IP_EVENT_STA_GOT_IP,
                &wifi_time_event_handler,
                nullptr);
            if (ret != ESP_OK) {
                ESP_UTILS_LOGE("Register time sync event handler failed: %s", esp_err_to_name(ret));
                return ret;
            }
            time_event_handler_registered = true;
        }

        esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("esp_wifi_set_mode returned: %s", esp_err_to_name(ret));
        }

        return ESP_OK;
    }
} // namespace esp_brookesia::apps
