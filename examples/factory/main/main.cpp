/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "bsp/esp-bsp.h"
#include "esp_brookesia.hpp"
#include "boost/thread.hpp"
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Main"
#include "esp_lib_utils.h"

#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "product_support.h"

#include "Drawpanel.hpp"
#include "SpecAnalyzer.hpp"
#include "MusicPlayer.hpp"
#include "Settings.hpp"
#include "Camera.hpp"
#include "VideoPlayer.hpp"
#include "esp_brookesia_app_calculator.hpp"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;

constexpr bool EXAMPLE_SHOW_MEM_INFO = true;

static bool is_wifi_connected()
{
    wifi_ap_record_t ap_info;
    return (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK);
}

extern "C" void app_main(void)
{
    esp_log_level_set("rpc_rsp", ESP_LOG_ERROR);
    const esp_partition_t *update_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
    );
    ESP_UTILS_CHECK_NULL_EXIT(update_partition, "Factory partition not found");
    ESP_LOGI(ESP_UTILS_LOG_TAG, "Switch to partition factory");
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(update_partition));

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(bsp_spiffs_mount());
    ESP_LOGI(ESP_UTILS_LOG_TAG, "SPIFFS mount successfully");

    ESP_ERROR_CHECK(product_codec_init());

    ESP_ERROR_CHECK(product_codec_volume_set(PRODUCT_CODEC_DEFAULT_VOLUME, NULL));

    ESP_UTILS_LOGI("Display ESP-Brookesia phone demo");

    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0}};

    // Keep LVGL on core 1 with a larger PSRAM-backed stack; high-resolution UI and
    // app switching can otherwise starve the GUI task or overflow the default stack.
    cfg.lv_adapter_cfg.task_stack_size = 16 * 1024;
    cfg.lv_adapter_cfg.task_priority = 10;
    cfg.lv_adapter_cfg.task_core_id = 1;
    cfg.lv_adapter_cfg.stack_in_psram = true;

    /* Configure display */
    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    (void)disp;
    bsp_display_backlight_off();

    /* Configure GUI lock */
    LvLock::registerCallbacks([](int timeout_ms) {
        esp_err_t ret = bsp_display_lock(timeout_ms);
        ESP_UTILS_CHECK_FALSE_RETURN(ret == ESP_OK, false, "Lock failed (timeout_ms: %d)", timeout_ms);

        return true;
    }, []() {
        bsp_display_unlock();
        return true;
    });

    /* Create a phone object */
    Phone *phone = new (std::nothrow) Phone();
    ESP_UTILS_CHECK_NULL_EXIT(phone, "Create phone failed");

    {
        // When operating on non-GUI tasks, should acquire a lock before operating on LVGL
        LvLockGuard gui_guard;

        /* Begin the phone */
        ESP_UTILS_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
        // assert(phone->getDisplay().showContainerBorder() && "Show container border failed");

        /* Init and install apps from registry */
        std::vector<systems::base::Manager::RegistryAppInfo> inited_apps;
        ESP_UTILS_CHECK_FALSE_EXIT(phone->initAppFromRegistry(inited_apps), "Init app registry failed");
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installAppFromRegistry(inited_apps), "Install app registry failed");

        auto app2 = esp_brookesia::apps::Calculator::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app2), "Start Calculator failed");
        auto app = esp_brookesia::apps::Drawpanel::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app), "Start Drawpanel failed");

        auto app1 = esp_brookesia::apps::SpecAnalyzer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app1), "Start SpecAnalyzer failed");
        auto app5 = esp_brookesia::apps::MusicPlayer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app5), "Start MusicPlayer failed");
        auto camera_app = esp_brookesia::apps::Camera::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(camera_app), "Start Camera failed");
        auto video_player_app = esp_brookesia::apps::VideoPlayer::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(video_player_app), "Start VideoPlayer failed");
        auto app6 = esp_brookesia::apps::Settings::requestInstance();
        ESP_UTILS_CHECK_FALSE_EXIT(phone->installApp(app6), "Start Settings failed");

        bsp_display_backlight_on();

        /* Create a timer to update the clock */
        lv_timer_create([](lv_timer_t *t) {
            time_t now;
            struct tm timeinfo;
            Phone *phone = (Phone *)t->user_data;

            ESP_UTILS_CHECK_NULL_EXIT(phone, "Invalid phone");

            time(&now);
            localtime_r(&now, &timeinfo);

            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min),
                "Refresh status bar failed"
            );

            bool connected = is_wifi_connected();
            StatusBar::WifiState wifi_state = connected ?
                StatusBar::WifiState::SIGNAL_3 :
                StatusBar::WifiState::DISCONNECTED;
                
            ESP_UTILS_CHECK_FALSE_EXIT(
                phone->getDisplay().getStatusBar()->setWifiIconState(wifi_state),
                "Refresh status bar failed"
            );
        }, 1000, phone);
    }

    if constexpr (EXAMPLE_SHOW_MEM_INFO) {
        esp_utils::thread_config_guard thread_config({
            .name = "mem_info",
            .stack_size = 4096,
        });
        boost::thread([ = ]() {
            char buffer[128];    /* Make sure buffer is enough for `sprintf` */
            size_t internal_free = 0;
            size_t internal_total = 0;
            size_t external_free = 0;
            size_t external_total = 0;

            while (1) {
                internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
                external_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                external_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
                snprintf(
                    buffer,
                    sizeof(buffer),
                    "\t           Biggest /     Free /    Total\n"
                    "\t  SRAM : [%8zu / %8zu / %8zu]\n"
                    "\t PSRAM : [%8zu / %8zu / %8zu]",
                    heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), internal_free, internal_total,
                    heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), external_free, external_total
                );
                ESP_UTILS_LOGI("\n%s", buffer);

                {
                    LvLockGuard gui_guard;
                    ESP_UTILS_CHECK_FALSE_EXIT(
                        phone->getDisplay().getRecentsScreen()->setMemoryLabel(
                            internal_free / 1024, internal_total / 1024, external_free / 1024, external_total / 1024
                        ), "Set memory label failed"
                    );
                }

                boost::this_thread::sleep_for(boost::chrono::seconds(5));
            }
        }).detach();
    }
}
