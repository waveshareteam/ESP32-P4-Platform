#include "AboutPage.hpp"
#include "esp_log.h"
#include "../Settings.hpp"

#define ESP_UTILS_LOG_TAG "BS:AboutPage"
#include "esp_lib_utils.h"

LV_IMG_DECLARE(logo);

namespace esp_brookesia::apps
{
    static constexpr int SETTINGS_PAGE_MARGIN = 40;
    static constexpr int SETTINGS_LIST_WIDTH = BSP_LCD_H_RES - SETTINGS_PAGE_MARGIN * 2;
    static constexpr int SETTINGS_LIST_HEIGHT = BSP_LCD_V_RES - 220;
    static constexpr int SETTINGS_ROW_HEIGHT = 88;

    AboutPage *AboutPage::_instance = nullptr;

    AboutPage *AboutPage::requestInstance(bool use_status_bar, bool use_navigation_bar)
    {
        if (_instance == nullptr)
        {
            _instance = new AboutPage(use_status_bar, use_navigation_bar);
        }
        return _instance;
    }

    AboutPage::AboutPage(bool use_status_bar, bool use_navigation_bar)
        : App("About", nullptr, true, use_status_bar, use_navigation_bar), label(nullptr)
    {
    }

    AboutPage::~AboutPage()
    {
    }

    bool AboutPage::run()
    {
        ESP_UTILS_LOGD("AboutPage Run");
        lv_obj_clean(lv_scr_act());

        lv_style_init(&style_list);
        lv_style_init(&style_list_btn);
        lv_style_init(&style_list_text);
        lv_style_init(&style_list_btn_pressed);

        // Match the root Settings list styling for a consistent page transition.
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

        static lv_style_t style_status;
        lv_style_init(&style_status);
        lv_style_set_text_font(&style_status, &lv_font_montserrat_30);
        lv_style_set_text_color(&style_status, lv_color_hex(0x00BFFF));
        lv_style_set_bg_color(&style_status, lv_color_hex(0x38393A));
        lv_style_set_bg_opa(&style_status, LV_OPA_COVER);
        lv_style_set_pad_all(&style_status, 10);

        lv_obj_t *status_btn = lv_list_add_btn(list1, LV_SYMBOL_LEFT, "back");
        lv_obj_add_style(status_btn, &style_status, 0);
        lv_obj_add_event_cb(status_btn, [](lv_event_t *e)
                            {
            (void)e;
            ESP_UTILS_LOGI("Goback clicked!");
            lv_async_call([](void *param) {
                (void)param;
                auto about = AboutPage::requestInstance(true, false);
                about->close();
                auto settings = Settings::requestInstance(true, false);
                settings->run();
            }, nullptr); }, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *text_obj = lv_list_add_text(list1, "Manufacturer");
        lv_obj_add_style(text_obj, &style_list_text, 0);

        lv_obj_t *wlan_btn = lv_list_add_btn(list1, &logo, "Waveshare");
        lv_obj_add_style(wlan_btn, &style_list_btn, 0);

        lv_obj_t *ui_obj = lv_list_add_text(list1, "UI Framework");
        lv_obj_add_style(ui_obj, &style_list_text, 0);
        lv_obj_t *ui_btn = lv_list_add_btn(list1, nullptr, "ESP-Brookesia");
        lv_obj_add_style(ui_btn, &style_list_btn, 0);

        lv_obj_clear_flag(wlan_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(ui_btn, LV_OBJ_FLAG_CLICKABLE);

        return true;
    }

    bool AboutPage::back()
    {
        ESP_UTILS_LOGD("AboutPage Back");
        return notifyCoreClosed();
    }

    bool AboutPage::close()
    {
        ESP_UTILS_LOGD("AboutPage Close");
        if (label)
        {
            lv_obj_del(label);
            label = nullptr;
        }
        if (list1)
        {
            lv_obj_del(list1);
            list1 = nullptr;
            lv_style_reset(&style_list);
            lv_style_reset(&style_list_btn);
            lv_style_reset(&style_list_text);
            lv_style_reset(&style_list_btn_pressed);
        }
        return true;
    }

} // namespace esp_brookesia::apps
