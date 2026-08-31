/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "product_support.h"
#include "./ui/AboutPage.hpp"
#include "./ui/DisplayPage.hpp"
#include "./ui/SoundPage.hpp"
#include "./ui/WlanPage.hpp"

namespace esp_brookesia::apps
{

    /**
     * @brief Settings application for displaying a list of options
     *
     */
    class Settings : public systems::phone::App
    {
    public:
        static Settings *requestInstance(bool use_status_bar = true, bool use_navigation_bar = false);
        ~Settings();
        bool run(void) override;
        i2c_master_bus_handle_t i2c_bus_handle;
        i2c_master_dev_handle_t pmu_dev_handle;
        static int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
        static int pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);

    protected:
        Settings(bool use_status_bar, bool use_navigation_bar);

        // bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        enum class ActivePage {
            None,
            Wlan,
            Sound,
            Display,
            About,
        };

        static void event_handler_cb(lv_event_t *e);
        static void open_page_async_cb(void *user_data);
        void add_button_with_arrow(const void *icon, const char *text);
        void create_settings_ui();
        void destroy_settings_ui();
        void event_handler(lv_event_t *e);
        void open_page(const char *page_name);

        esp_err_t initWifi();
        
    private:
        static Settings *_instance;
        lv_obj_t *list1;
        lv_style_t style_list;
        lv_style_t style_list_btn;
        lv_style_t style_list_text;
        lv_style_t style_list_btn_pressed;
        ActivePage active_page;
    };

} // namespace esp_brookesia::apps
