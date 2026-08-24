#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "bsp/esp-bsp.h"
#include "product_support.h"
#include "driver/ppa.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"

namespace esp_brookesia::apps
{

    class Camera : public systems::phone::App
    {
    public:
        static Camera *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~Camera();

    protected:
        Camera(bool use_status_bar, bool use_navigation_bar);

        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        static Camera *_instance;

        static constexpr int CAMERA_BUFFER_COUNT = 2;
        static constexpr int PREVIEW_TASK_STACK_SIZE = 8 * 1024;
        static constexpr int PREVIEW_TASK_PRIORITY = 8;
        static constexpr int SWIPE_EXIT_THRESHOLD = 180;

        lv_obj_t *_status_label = nullptr;
        lv_display_t *_display = nullptr;
        esp_lcd_panel_handle_t _display_panel = nullptr;
        ppa_client_handle_t _ppa_srm_handle = nullptr;
        esp_lcd_touch_handle_t _touch_handle = nullptr;

        TaskHandle_t _preview_task_handle = nullptr;
        TaskHandle_t _close_wait_task = nullptr;
        int _video_fd = -1;
        bool _video_driver_initialized = false;
        bool _preview_running = false;
        bool _dummy_enabled = false;
        bool _lvgl_paused = false;
        bool _touch_active = false;
        uint16_t _touch_start_x = 0;
        uint16_t _touch_start_y = 0;
        uint32_t _camera_width = 0;
        uint32_t _camera_height = 0;
        size_t _camera_buffer_size = 0;
        size_t _camera_buffer_lengths[CAMERA_BUFFER_COUNT] = {};
        size_t _data_cache_line_size = 64;
        void *_camera_buffers[CAMERA_BUFFER_COUNT] = {};
        void *_lcd_buffers[CONFIG_BSP_LCD_DPI_BUFFER_NUMS] = {};

        bool startPreview();
        void requestStopPreview();
        esp_err_t initVideoDriver();
        esp_err_t openVideoDevice();
        esp_err_t setupCameraBuffers();
        esp_err_t startDummyPreview();
        void stopDummyPreview();
        void releaseCameraBuffers();
        bool shouldExitBySwipe();
        esp_err_t handleFrame();
        void previewTask();

        static void previewTaskEntry(void *arg);
    };

} // namespace esp_brookesia::apps
