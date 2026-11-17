#include "WlanPage.hpp"
#include "../Settings.hpp"

#define ESP_UTILS_LOG_TAG "BS:WlanPage"
#include "esp_lib_utils.h"
#include "nvs.h"

#include <stdlib.h>

LV_IMG_DECLARE(wifi_1);
LV_IMG_DECLARE(wifi_2);
LV_IMG_DECLARE(wifi_3);
LV_IMG_DECLARE(wifi_4);

namespace esp_brookesia::apps
{
    static constexpr int SETTINGS_PAGE_MARGIN = 40;
    static constexpr int SETTINGS_LIST_WIDTH = BSP_LCD_H_RES - SETTINGS_PAGE_MARGIN * 2;
    static constexpr int SETTINGS_LIST_HEIGHT = BSP_LCD_V_RES - 220;
    static constexpr int SETTINGS_ROW_HEIGHT = 88;

    WlanPage *WlanPage::_instance = nullptr;

    lv_obj_t *WlanPage::ta = nullptr;
    lv_obj_t *WlanPage::kb = nullptr;

    WlanPage *WlanPage::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new WlanPage(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    WlanPage::WlanPage(bool use_status_bar, bool use_navigation_bar)
        : App("WLAN", nullptr, true, use_status_bar, use_navigation_bar),
          label(nullptr), list1(nullptr),
          connected_text(nullptr), conn_btn(nullptr), available_text(nullptr)
    {
        status_btn = nullptr;
        wlan_switch = nullptr;
        wifi_icon = nullptr;
        spinner = nullptr;
        wifi_TaskHandle = nullptr;
        wifi_index = 0;
        wifi_ssid = nullptr;
        wifi_pwd = nullptr;
    }

    WlanPage::~WlanPage() {}

    bool WlanPage::run()
    {
        ESP_UTILS_LOGI("WlanPage Run");
        page_active = true;

        _nvs_param_map[NVS_KEY_WIFI_ENABLE] = false;
        loadNvsParam();

        start_wifi_events();
        esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
        }
        ret = esp_wifi_start();
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("esp_wifi_start failed: %s", esp_err_to_name(ret));
        }

        CreateWifiUI();

        // Restore the persisted Wi-Fi switch state after the UI exists.
        if (_nvs_param_map[NVS_KEY_WIFI_ENABLE]) {
            OpenWifi();
        } 

        // The background task owns blocking scan/connect calls and posts UI work
        // back to LVGL with lv_async_call.
        if (wifi_TaskHandle == nullptr) {
            wifi_task_running = true;
            BaseType_t task_ret = xTaskCreate(wifi_task, "wifi_task", 6 * 1024, this, 9, &wifi_TaskHandle);
            if (task_ret != pdPASS) {
                wifi_task_running = false;
                wifi_TaskHandle = nullptr;
                ESP_UTILS_LOGE("Create Wi-Fi task failed");
                return false;
            }
        }

        return true;
    }

    bool WlanPage::back()
    {
        ESP_UTILS_LOGI("WlanPage Back");
        
        return notifyCoreClosed();
    }

    bool WlanPage::close()
    {
        ESP_UTILS_LOGI("WlanPage Close");
        page_active = false;
        wifi_task_running = false;
        esp_wifi_scan_stop();
        for (int i = 0; wifi_TaskHandle && i < 50; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (wifi_TaskHandle) {
            ESP_UTILS_LOGW("wifi_task did not exit before page close finished");
        }

        if(wifi_events_registered)
            stop_wifi_events();
        if (label)
        {
            lv_obj_del(label);
            label = nullptr;
        }
        if (spinner)
        {
            lv_obj_del(spinner);
            spinner = nullptr;
        }
        if (kb)
        {
            lv_obj_del(kb);
            kb = nullptr;
        }
        if (ta)
        {
            lv_obj_del(ta);
            ta = nullptr;
        }
        if (list1)
        {
            lv_obj_del(list1);
            list1 = nullptr;
            connected_text = nullptr;
            conn_btn = nullptr;
            available_text = nullptr;
            status_btn = nullptr;
            wlan_switch = nullptr;
            wifi_icon = nullptr;
            spinner = nullptr;
            wifi_btns.clear();
            lv_style_reset(&style_list);
            lv_style_reset(&style_list_btn);
            lv_style_reset(&style_list_text);
            lv_style_reset(&style_list_btn_pressed);
        }

        return true;
    }


    void WlanPage::CreateWifiUI()
    {
        lv_obj_clean(lv_scr_act());

        lv_style_init(&style_list);
        lv_style_init(&style_list_btn);
        lv_style_init(&style_list_text);
        lv_style_init(&style_list_btn_pressed);

        lv_style_set_pad_all(&style_list, 5);
        lv_style_set_pad_row(&style_list, 2);
        lv_style_set_border_width(&style_list, 0);
        lv_style_set_radius(&style_list, 10);
        lv_style_set_bg_color(&style_list, lv_color_hex(0x000000));
        lv_style_set_bg_opa(&style_list, LV_OPA_COVER);

        lv_style_set_height(&style_list_btn, SETTINGS_ROW_HEIGHT);
        lv_style_set_pad_all(&style_list_btn, 16);
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

        static lv_style_t style_status;
        lv_style_init(&style_status);
        lv_style_set_text_font(&style_status, &lv_font_montserrat_30);
        lv_style_set_text_color(&style_status, lv_color_hex(0x00BFFF));
        lv_style_set_bg_color(&style_status, lv_color_hex(0x38393A));
        lv_style_set_bg_opa(&style_status, LV_OPA_COVER);
        lv_style_set_pad_all(&style_status, 10);

        status_btn = lv_list_add_btn(list1, LV_SYMBOL_LEFT, "back");
        lv_obj_add_style(status_btn, &style_status, 0);
        lv_obj_add_event_cb(status_btn, [](lv_event_t *e)
                            {
        (void)e;
        ESP_UTILS_LOGI("Goback clicked!");
        lv_async_call([](void *param) {
            (void)param;
            auto wlan = WlanPage::requestInstance(true, false);
            wlan->close();
            auto settings = Settings::requestInstance(true, false);
            settings->run();
        }, nullptr); }, LV_EVENT_CLICKED, nullptr);
         
        lv_obj_t * wifi_ant_btn = lv_button_create(status_btn);
        lv_obj_add_style(wifi_ant_btn, &style_status, 0);
        lv_obj_set_user_data(wifi_ant_btn, (void*)0);
        
        lv_obj_add_event_cb(wifi_ant_btn, [](lv_event_t *e)
        {
            (void)e;
        }, LV_EVENT_CLICKED, nullptr);

        lv_obj_t *text_obj = lv_list_add_text(list1, "Status");
        lv_obj_add_style(text_obj, &style_list_text, 0);

        lv_obj_t *wlan_btn = lv_list_add_btn(list1, nullptr, "WLAN");
        lv_obj_add_style(wlan_btn, &style_list_btn, 0);

        wlan_switch = lv_switch_create(wlan_btn);
        lv_obj_align_to(wlan_switch, wlan_btn, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_remove_state(wlan_switch, LV_STATE_CHECKED);
        
        lv_obj_add_event_cb(wlan_switch, [](lv_event_t *e)
        {
            lv_obj_t *sw = static_cast<lv_obj_t *>(lv_event_get_target(e));
            bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
            auto page = WlanPage::requestInstance();

            if (checked) page->toggleWifiUI(WIFIOPEN);
            else page->toggleWifiUI(WIFICLOSE);
        }, 
        LV_EVENT_VALUE_CHANGED, nullptr);

        lv_obj_add_state(wlan_btn, LV_STATE_DISABLED);
        // lv_obj_add_state(conn_btn, LV_STATE_DISABLED);

        spinner = lv_spinner_create(lv_scr_act()); // Create a spinner with 1000ms rotation duration
        lv_obj_set_size(spinner, 96, 96);
        lv_obj_set_pos(spinner, 0, 80);
        lv_obj_set_align(spinner, LV_ALIGN_CENTER);              // Align spinner to the center of the parent container
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);            // Initially hide the spinner
        lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
        lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);

        ta = lv_textarea_create(lv_scr_act());
        lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(ta, BSP_LCD_H_RES - 160, 72);
        lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 120);

        lv_textarea_set_password_mode(ta, true);
        lv_textarea_set_password_show_time(ta,1500);
        lv_textarea_set_placeholder_text(ta, "Enter password...");

        lv_obj_set_style_border_color(ta, lv_color_hex(0x555555), LV_PART_MAIN);
        lv_obj_set_style_border_width(ta, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(ta, 5, LV_PART_MAIN);
        lv_obj_set_style_text_color(ta, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

        lv_obj_set_style_bg_color(ta, lv_color_hex(0x181818), static_cast<lv_style_prop_t>(LV_PART_CURSOR) | static_cast<lv_style_prop_t>(LV_STATE_DEFAULT));

        lv_obj_set_style_text_color(ta, lv_color_hex(0xAAAAAA), LV_PART_TEXTAREA_PLACEHOLDER);

        lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_TEXTAREA_PLACEHOLDER);

        lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, this);

        kb = lv_keyboard_create(lv_scr_act());
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(kb, BSP_LCD_H_RES - 80, 420);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -40);

        lv_obj_set_style_bg_color(kb, lv_color_hex(0x181818), 0);
        lv_obj_set_style_border_color(kb, lv_color_hex(0x555555), 0);
        lv_obj_set_style_border_width(kb, 2, 0);
        lv_obj_set_style_radius(kb, 5, 0);

        // LVGL keyboard buttons are child objects; style them after creation.
        uint32_t child_cnt = lv_obj_get_child_cnt(kb);
        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * row = lv_obj_get_child(kb, i);
            uint32_t btn_cnt = lv_obj_get_child_cnt(row);
            for(uint32_t j = 0; j < btn_cnt; j++) {
                lv_obj_t * btn = lv_obj_get_child(row, j);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_border_color(btn, lv_color_hex(0x555555), 0);
                lv_obj_set_style_border_width(btn, 1, 0);
                lv_obj_set_style_radius(btn, 5, 0);
            }
        }
        lv_keyboard_set_textarea(kb, ta);

        lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, this);

        lv_obj_set_scroll_dir(lv_scr_act(), LV_DIR_VER);
        lv_obj_scroll_to_view(ta, LV_ANIM_ON);
    }

    bool WlanPage::OpenWifi()
    {
        ESP_UTILS_LOGI("WlanPage OpenWifi");

        if (!wifi_events_registered) start_wifi_events();

        if (wlan_switch) {
            lv_obj_add_state(wlan_switch, LV_STATE_CHECKED);
            lv_obj_add_state(wlan_switch, LV_STATE_DISABLED);
        }
        if (status_btn) {
            lv_obj_add_state(status_btn, LV_STATE_DISABLED);
        }
        if (spinner) {
            lv_obj_remove_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        }
        
        Wifi_state = WIFIOPEN;
        
        return true;
    }

    bool WlanPage::CloseWifi()
    {
        ESP_UTILS_LOGI("WlanPage CloseWifi");
        Wifi_state = WIFICLOSE;

        esp_wifi_disconnect();   
        
        if (connected_text)
            lv_obj_add_flag(connected_text, LV_OBJ_FLAG_HIDDEN);
        if (conn_btn)
            lv_obj_add_flag(conn_btn, LV_OBJ_FLAG_HIDDEN);
        if (available_text)
            lv_obj_add_flag(available_text, LV_OBJ_FLAG_HIDDEN);
        for (auto btn : wifi_btns)
        {
            if (btn)
                lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        }

        return true;
    }

    void WlanPage::toggleWifiUI(WifiState visible)
    {
        if (visible == WIFIOPEN) 
        {
            _nvs_param_map[NVS_KEY_WIFI_ENABLE] = true;
            setNvsParam(NVS_KEY_WIFI_ENABLE, true);
            printf("WIFIOPEN \r\n");
            OpenWifi();
        }
        else if(visible == WIFICLOSE) {
            printf("WIFICLOSE \r\n");
            _nvs_param_map[NVS_KEY_WIFI_ENABLE] = false;
            setNvsParam(NVS_KEY_WIFI_ENABLE, false);
            CloseWifi();
        }
    }
    // Runs on the LVGL thread after Wi-Fi state changes.
    void WlanPage::wifi_state_cb(void *param)
    {
        WlanPage *self = static_cast<WlanPage *>(param);
        if (!self || !self->page_active || !self->list1) {
            return;
        }
        if (self->kb) {
            lv_obj_add_flag(self->kb, LV_OBJ_FLAG_HIDDEN);
        }
        if (self->ta) {
            lv_obj_add_flag(self->ta, LV_OBJ_FLAG_HIDDEN);
        }
        if (self->spinner) {
            lv_obj_add_flag(self->spinner, LV_OBJ_FLAG_HIDDEN);
        }
        if (self->wlan_switch) {
            lv_obj_remove_state(self->wlan_switch, LV_STATE_DISABLED);
        }
        if (self->status_btn) {
            lv_obj_remove_state(self->status_btn, LV_STATE_DISABLED);
        }
        

        lv_timer_t *t = lv_timer_create(self->wifi_sta_cb, 100, self);
        if (t) {
            lv_timer_set_repeat_count(t, 1);
        } else {
            ESP_UTILS_LOGW("Create Wi-Fi state refresh timer failed");
        }
    }

    // Handles AP list button events and owns the per-button user_data lifetime.
    void WlanPage::wifi_btn_cb(lv_event_t * e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        btn_user_data_t *ud = (btn_user_data_t *)lv_event_get_user_data(e);
        if (!ud) {
            return;
        }
        if (event_code == LV_EVENT_DELETE) {
            free(ud);
            return;
        }
        WlanPage *page = static_cast<WlanPage *>(ud->self);
        if (!page || !page->page_active) {
            return;
        }
        // When a network in the list is pressed
        if(event_code == LV_EVENT_SHORT_CLICKED) {
            page->wifi_index = ud->index;

            ESP_UTILS_LOGI("wifi_index %d", page->wifi_index);

            page->wifi_ssid = page->ap_info[page->wifi_index].ssid;
            ESP_UTILS_LOGI("Selected SSID: %s", (const char *)page->wifi_ssid);
            
            lv_obj_add_flag(page->list1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ta, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ta, LV_STATE_FOCUSED);
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(kb, ta);

        }
    }

    // Rebuilds the connected/available AP list after a background scan completes.
    void WlanPage::wifi_scan_cb(lv_timer_t * timer) {
        
        WlanPage *self = static_cast<WlanPage *>(timer->user_data);
        if (!self || !self->page_active || !self->list1) {
            return;
        }

        if (self->connected_text) {
            lv_obj_del(self->connected_text);
            self->connected_text = nullptr;
        }
        if (self->conn_btn) {
            lv_obj_del(self->conn_btn);
            self->conn_btn = nullptr;
            self->wifi_icon = nullptr;
        }
        if (self->available_text) {
            lv_obj_del(self->available_text);
            self->available_text = nullptr;
        }
        for (auto btn : self->wifi_btns) {
            if (btn) {
                lv_obj_del(btn);
            }
        }
        self->wifi_btns.clear();

        // Connected WLAN
        self->connected_text = lv_list_add_text(self->list1, "Connected WLAN");
        lv_obj_add_style(self->connected_text, &self->style_list_text, 0);
        lv_obj_add_flag(self->connected_text, LV_OBJ_FLAG_HIDDEN);

        self->conn_btn = lv_list_add_btn(self->list1, nullptr,"SSID");
        lv_obj_add_style(self->conn_btn, &self->style_list_btn, 0);
        lv_obj_add_flag(self->conn_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(self->conn_btn, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_add_event_cb(self->conn_btn, [](lv_event_t *e)
        {
            lv_event_code_t event_code = lv_event_get_code(e);
            auto page = WlanPage::requestInstance();

            // When a network in the list is pressed
            if(event_code == LV_EVENT_SHORT_CLICKED) {
                ESP_LOGI("WIFI", "LV_EVENT_SHORT_CLICKED");
                esp_err_t ret = esp_wifi_restore();
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("esp_wifi_restore failed: %s", esp_err_to_name(ret));
                }
                ret = esp_wifi_set_mode(WIFI_MODE_STA);
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("esp_wifi_set_mode failed: %s", esp_err_to_name(ret));
                }
                ret = esp_wifi_start();
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("esp_wifi_start failed: %s", esp_err_to_name(ret));
                }
                page->Wifi_state = DISCONNECT;
                page->connection_num = 0;
                if (page->connected_text) {
                    lv_obj_add_flag(page->connected_text, LV_OBJ_FLAG_HIDDEN);
                }
                if (page->conn_btn) {
                    lv_obj_add_flag(page->conn_btn, LV_OBJ_FLAG_HIDDEN);
                }
            }
        }, 
        LV_EVENT_SHORT_CLICKED, nullptr);

        self->wifi_icon = lv_label_create(self->conn_btn);
        lv_label_set_text(self->wifi_icon, LV_SYMBOL_CLOSE); //LV_SYMBOL_OK
        lv_obj_align(self->wifi_icon, LV_ALIGN_RIGHT_MID, -20, 0);
        lv_obj_set_style_text_font(self->wifi_icon, &lv_font_montserrat_30, 0);
        // Available WLAN
        self->available_text = lv_list_add_text(self->list1, "Available WLAN");
        lv_obj_add_style(self->available_text, &self->style_list_text, 0);

        uint16_t display_count = self->scanned_ap_count;
        if (display_count > DEFAULT_SCAN_LIST_SIZE) {
            display_count = DEFAULT_SCAN_LIST_SIZE;
        }

        for (int i = 0; i < display_count; i++)
        {
            if (self->ap_info[i].ssid[0] == '\0') {
                continue;
            }
            lv_obj_t *wifi_btn = lv_list_add_btn(self->list1, nullptr, (const char *)self->ap_info[i].ssid);
            lv_obj_add_style(wifi_btn, &self->style_list_btn, 0);
            lv_obj_add_style(wifi_btn, &self->style_list_btn_pressed, LV_STATE_PRESSED);
            
            lv_obj_t *icon = lv_img_create(wifi_btn);
            if (self->ap_info[i].rssi > -25)
                lv_img_set_src(icon, &wifi_4);
            else if ((self->ap_info[i].rssi < -25) && (self->ap_info[i].rssi > -50))  // Medium signal
                // Add button with medium signal icon
               lv_img_set_src(icon, &wifi_3);
            else if ((self->ap_info[i].rssi < -50) && (self->ap_info[i].rssi > -75))  // Medium signal
                // Add button with weak signal icon
                lv_img_set_src(icon, &wifi_2);
            else lv_img_set_src(icon, &wifi_1);

            btn_user_data_t *ud = (btn_user_data_t *)malloc(sizeof(btn_user_data_t));
            if (!ud) {
                ESP_UTILS_LOGW("Allocate Wi-Fi button user data failed");
                lv_obj_del(wifi_btn);
                continue;
            }

            ud->index = i;
            ud->self = self;
            lv_obj_add_event_cb(wifi_btn,wifi_btn_cb, LV_EVENT_ALL, ud);
            lv_obj_align(icon, LV_ALIGN_RIGHT_MID, 0, 0);

            self->wifi_btns.push_back(wifi_btn);
        }

        lv_obj_remove_state(self->wlan_switch, LV_STATE_DISABLED);
        lv_obj_remove_state(self->status_btn, LV_STATE_DISABLED);
        lv_obj_add_flag(self->spinner, LV_OBJ_FLAG_HIDDEN); 

        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI("WIFI", "Connected to SSID: %s, RSSI: %d", ap_info.ssid, ap_info.rssi);
            lv_obj_remove_flag(self->connected_text, LV_OBJ_FLAG_HIDDEN);
            lv_list_set_button_text(self->list1, self->conn_btn, (const char*)ap_info.ssid);
            lv_obj_remove_flag(self->conn_btn, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(self->wifi_icon, LV_SYMBOL_OK);
        } else {
            ESP_LOGI("WIFI", "Not connected");
            static wifi_config_t wifi_config;
            esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
            if (ret != ESP_OK) {
                ESP_UTILS_LOGW("esp_wifi_get_config failed: %s", esp_err_to_name(ret));
                return;
            }
            ESP_UTILS_LOGI("SSID stored in NVS: %s", wifi_config.sta.ssid);
            if (wifi_config.sta.ssid[0] != '\0' && self->_nvs_param_map[NVS_KEY_WIFI_ENABLE]) {
                printf("Wi-Fi ssid exists\n");
                self->wifi_ssid = wifi_config.sta.ssid;

                lv_obj_remove_flag(self->spinner, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_state(self->wlan_switch, LV_STATE_DISABLED);
                lv_obj_add_state(self->status_btn, LV_STATE_DISABLED);

                ret = esp_wifi_connect();
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("esp_wifi_connect failed: %s", esp_err_to_name(ret));
                    lv_async_call(wifi_state_cb, self);
                }
            } else {
                printf("Wi-Fi ssid empty\n");
            }   
        } 
    }

    // One-shot UI refresh after a connect attempt completes or times out.
    void WlanPage::wifi_sta_cb(lv_timer_t * timer) {
        
        WlanPage *self = static_cast<WlanPage *>(timer->user_data);
        if (!self || !self->page_active || !self->list1 || !self->connected_text || !self->conn_btn || !self->wifi_icon) {
            return;
        }
        
        lv_obj_remove_flag(self->connected_text, LV_OBJ_FLAG_HIDDEN);
        lv_list_set_button_text(self->list1, self->conn_btn, (const char*)self->wifi_ssid);
        if (self->Wifi_state == CONNECTED) 
        {
            lv_label_set_text(self->wifi_icon, LV_SYMBOL_OK);
            lv_obj_remove_flag(self->conn_btn, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(self->conn_btn, LV_OBJ_FLAG_HIDDEN);
            
        }
        else 
        {
            lv_label_set_text(self->wifi_icon, LV_SYMBOL_CLOSE);
            lv_obj_add_flag(self->conn_btn, LV_OBJ_FLAG_CLICKABLE);
        }
    }

    // Keyboard READY/CANCEL hides the input and lets wifi_task perform the blocking connect.
    void WlanPage::kb_event_cb(lv_event_t *e)
    {
        WlanPage *self = (WlanPage *)lv_event_get_user_data(e);
        lv_event_code_t code = lv_event_get_code(e);

        if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
            
            self->wifi_pwd = (uint8_t *)lv_textarea_get_text(ta);
            
            
            ESP_UTILS_LOGI("Wi-Fi password length: %u", static_cast<unsigned>(strlen((char *)self->wifi_pwd)));
            if (strlen((char *)self->wifi_pwd) >= 8)
            {
                self->Wifi_state = CONNECTING;
            }
            else
            {
                self->Wifi_state = DISCONNECT;
                lv_async_call(wifi_state_cb, self);
            }
            lv_obj_add_flag(self->kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(self->ta, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(self->spinner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(self->wlan_switch, LV_STATE_DISABLED);
            lv_obj_add_state(self->status_btn, LV_STATE_DISABLED);
            lv_obj_remove_flag(self->list1, LV_OBJ_FLAG_HIDDEN);
  
        }
    }

    // Textarea focus controls keyboard visibility.
    void WlanPage::ta_event_cb(lv_event_t * e) 
    {
        WlanPage *self = (WlanPage *)lv_event_get_user_data(e);
        lv_obj_t * ta = (lv_obj_t*)lv_event_get_target(e);

        if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(kb, ta);
        }
        else if (lv_event_get_code(e) == LV_EVENT_DEFOCUSED) {
            lv_obj_remove_flag(self->list1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ta, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(kb, NULL);
        }
    }

    // ESP event loop callback. UI work is deferred with lv_async_call because the
    // event loop does not run on the LVGL task.
    void WlanPage::wifi_event_handler(void *arg, esp_event_base_t event_base,\
                                int32_t event_id, void *event_data)
    {
        WlanPage *self = static_cast<WlanPage *>(arg);
        if (!self) {
            return;
        }
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            // esp_wifi_connect();
            ESP_UTILS_LOGI("WIFI_EVENT_STA_START");
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if (self->Wifi_state == WIFICLOSE || !self->_nvs_param_map[NVS_KEY_WIFI_ENABLE]) {
                self->connection_num = 0;
                return;
            }
            if (self->connection_num < 3)
            {
                esp_wifi_connect();
                ESP_UTILS_LOGI("esp_wifi_connect");
            }
            else
            {
                self->connection_num = 0;
                ESP_UTILS_LOGI("connection_fail");
                self->Wifi_state = DISCONNECT;
                lv_async_call(wifi_state_cb, self);
            }
            self->connection_num++;
            

        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            // ESP_UTILS_LOGI("IP_EVENT_STA_GOT_IP");
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_UTILS_LOGI("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            self->Wifi_state = CONNECTED;
            self->connection_num = 0;

            lv_async_call(wifi_state_cb, self);
        }

    }

    void WlanPage::start_wifi_events() {
        if (wifi_events_registered) {
            return;
        }
        // Register the Wi-Fi event handler
        esp_err_t ret = esp_event_handler_register(WIFI_EVENT,
                                                   ESP_EVENT_ANY_ID,
                                                   &wifi_event_handler,
                                                   this);
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("Register Wi-Fi event handler failed: %s", esp_err_to_name(ret));
            return;
        }

        ret = esp_event_handler_register(IP_EVENT,
                                         IP_EVENT_STA_GOT_IP,
                                         &wifi_event_handler,
                                         this);
        if (ret != ESP_OK) {
            ESP_UTILS_LOGW("Register IP event handler failed: %s", esp_err_to_name(ret));
            esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
            return;
        }

        wifi_events_registered = true;                                                                                              
        ESP_UTILS_LOGI("Wi-Fi event handler registered.\n");
    }

    void WlanPage::stop_wifi_events() {
        esp_err_t ret = esp_event_handler_unregister(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &wifi_event_handler);
        if (ret != ESP_OK)
            ESP_UTILS_LOGI("Wi-Fi event handler unregistered Faile.\n");

            
        ret = esp_event_handler_unregister(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &wifi_event_handler);
        if (ret != ESP_OK)
            ESP_UTILS_LOGI("IP event handler unregistered Faile.\n");                                            

        wifi_events_registered = false;
        ESP_UTILS_LOGI("Wi-Fi event handler unregistered.\n");
    }

    // Background Wi-Fi worker. It serializes scan and connect operations so the UI
    // thread is never blocked by esp_wifi_scan_start(..., true).
    void WlanPage::wifi_task(void *arg)
    {
        WlanPage *self = static_cast<WlanPage*>(arg);
        if (!self) return;

        while (self->wifi_task_running)
        {
            // ESP_UTILS_LOGI("Failed to get STA list");
            if (self->Wifi_state == WIFIOPEN)
            {
                self->Wifi_state = WIFISCAN;

                uint16_t number = DEFAULT_SCAN_LIST_SIZE;  // Maximum number of APs to scan for
                uint16_t ap_count = 0;  // Variable to hold the number of found APs

                // Start the Wi-Fi scan
                esp_err_t ret = esp_wifi_scan_start(NULL, true);
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("esp_wifi_scan_start failed: %s", esp_err_to_name(ret));
                    self->Wifi_state = WIFICLOSE;
                    lv_async_call(wifi_state_cb, self);
                    vTaskDelay(pdMS_TO_TICKS(300));
                    continue;
                }

                // Clear the ap_info array to hold fresh scan results
                memset(self->ap_info, 0, sizeof(self->ap_info));

                ESP_UTILS_LOGI("Max AP number ap_info can hold = %u", number);
                ret = esp_wifi_scan_get_ap_num(&ap_count);
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("esp_wifi_scan_get_ap_num failed: %s", esp_err_to_name(ret));
                    ap_count = 0;
                }
                ret = esp_wifi_scan_get_ap_records(&number, self->ap_info);
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("esp_wifi_scan_get_ap_records failed: %s", esp_err_to_name(ret));
                    number = 0;
                }
                self->scanned_ap_count = number;
                ESP_UTILS_LOGI("Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

                if (self->page_active) {
                    lv_async_call([](void *param) {
                        auto page = static_cast<WlanPage *>(param);
                        if (!page || !page->page_active || !page->list1) {
                            return;
                        }
                        lv_timer_t *t = lv_timer_create(page->wifi_scan_cb, 20, page);
                        if (t) {
                            lv_timer_set_repeat_count(t, 1);
                        } else {
                            ESP_UTILS_LOGW("Create Wi-Fi scan refresh timer failed");
                        }
                    }, self);
                }
            }
            
            if (self->Wifi_state == CONNECTING)
            {
                ESP_UTILS_LOGI("ssid:%s", self->wifi_ssid);
                ESP_UTILS_LOGI("password length:%u", static_cast<unsigned>(strlen((char *)self->wifi_pwd)));

                // ESP_UTILS_LOGI("authmode:%d", self->ap_info[self->wifi_index].authmode);
                esp_wifi_disconnect();

                wifi_config_t wifi_config = {};
                strlcpy((char *)wifi_config.sta.ssid, (const char *)self->wifi_ssid, sizeof(wifi_config.sta.ssid));
                strlcpy((char *)wifi_config.sta.password, (const char *)self->wifi_pwd, sizeof(wifi_config.sta.password));
                wifi_config.sta.threshold.authmode = self->ap_info[self->wifi_index].authmode;
                
                // Set WiFi configuration
                esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
                if (ret == ESP_OK) {
                    ret = esp_wifi_connect();
                }
                if (ret != ESP_OK) {
                    ESP_UTILS_LOGW("Connect WiFi failed: %s", esp_err_to_name(ret));
                    self->Wifi_state = DISCONNECT;
                    lv_async_call(wifi_state_cb, self);
                }

                self->Wifi_state = DISCONNECT;
            }

            // Delay for 10ms before the next loop iteration
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        self->wifi_TaskHandle = nullptr;
        vTaskDelete(NULL);
    }

    // Load or create persisted page settings.
    bool WlanPage::loadNvsParam(void)
    {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
            return false;
        }

        for (auto& key_value : _nvs_param_map) {
            int32_t val = key_value.second;
            err = nvs_get_i32(nvs_handle, key_value.first.c_str(), &val);
            if (err == ESP_OK) {
                key_value.second = val;
                ESP_LOGI("NVS", "Load %s: %ld", key_value.first.c_str(), key_value.second);
            } else if (err == ESP_ERR_NVS_NOT_FOUND) {
                err = nvs_set_i32(nvs_handle, key_value.first.c_str(), val);
                if (err != ESP_OK) {
                    ESP_LOGE("NVS", "Error (%s) setting default for %s", esp_err_to_name(err), key_value.first.c_str());
                    nvs_close(nvs_handle);
                    return false;
                }
                ESP_LOGW("NVS", "Key %s not found, set default: %ld", key_value.first.c_str(), val);
            } else {
                ESP_LOGE("NVS", "Error (%s) reading key %s", esp_err_to_name(err), key_value.first.c_str());
                nvs_close(nvs_handle);
                return false;
            }
        }

        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) committing NVS changes", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return false;
        }

        nvs_close(nvs_handle);
        return true;
    }

    bool WlanPage::setNvsParam(std::string key, int value)
    {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(err));
            return false;
        }

        int32_t v = value;
        err = nvs_set_i32(nvs_handle, key.c_str(), v);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) setting %s", esp_err_to_name(err), key.c_str());
            nvs_close(nvs_handle);
            return false;
        }

        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE("NVS", "Error (%s) committing NVS changes", esp_err_to_name(err));
            nvs_close(nvs_handle);
            return false;
        }

        ESP_LOGI("NVS", "Set %s = %d", key.c_str(), value);
        nvs_close(nvs_handle);
        return true;
    }
} // namespace esp_brookesia::apps
