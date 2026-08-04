#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_lcd_mipi_dsi.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "lv_demos.h"

static const char *TAG = "app_main";

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0
        }};
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();  /* Best-effort — may fail silently without 0x45 */

    /* Hardware test pattern for 3 s to verify MIPI DSI + display init */
    esp_lcd_panel_handle_t panel = bsp_display_get_panel_handle();
    if (panel) {
        ESP_LOGI(TAG, "Hardware pattern on for 3s (should be visible if display+backlight work)");
        esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_BAR_VERTICAL);
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_lcd_dpi_panel_set_pattern(panel, MIPI_DSI_PATTERN_NONE);
    }

    bsp_display_lock(-1);
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_make(0, 0, 180), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "DISPLAY WORKING!");
    lv_obj_set_style_text_color(label, lv_color_make(255, 255, 0), 0);
    lv_obj_center(label);
    bsp_display_unlock();
    ESP_LOGI(TAG, "UI created");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "alive");
    }
}