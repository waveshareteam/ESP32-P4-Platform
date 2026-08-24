#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "product_support.h"
#include "driver/ppa.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_types.h"
#include "sdkconfig.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "avi_player.h"
#include "esp_jpeg_dec.h"

namespace esp_brookesia::apps
{

    class VideoPlayer : public systems::phone::App
    {
    public:
        static VideoPlayer *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~VideoPlayer();

    protected:
        VideoPlayer(bool use_status_bar, bool use_navigation_bar);
        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        static VideoPlayer *_instance;

        static constexpr int SWIPE_EXIT_THRESHOLD = 180;

        lv_display_t *display = nullptr;
        esp_lcd_panel_handle_t display_panel = nullptr;
        ppa_client_handle_t ppa_srm_handle = nullptr;
        esp_lcd_touch_handle_t touch_handle = nullptr;
        void *lcd_buffers[CONFIG_BSP_LCD_DPI_BUFFER_NUMS] = {};
        int current_buf_idx = 0;
        uint8_t *jpeg_output_buffer = nullptr;
        int jpeg_output_buffer_size = 0;
        size_t data_cache_line_size = 64;
        uint16_t last_video_width = 0;
        uint16_t last_video_height = 0;
        bool loop_playback = true;
        bool is_playing = false;
        bool is_paused = false;
        bool sd_mounted = false;
        bool dummy_enabled = false;
        bool lvgl_paused = false;
        bool touch_active = false;
        uint16_t touch_start_x = 0;
        uint16_t touch_start_y = 0;

        TaskHandle_t play_task_handle = nullptr;
        TaskHandle_t close_wait_task = nullptr;

        avi_player_handle_t avi_player_handle;
        jpeg_dec_handle_t jpeg_handle = nullptr;

        char **avi_file_list = nullptr;
        int avi_file_count = 0;

        lv_obj_t *status_label = nullptr;

        esp_err_t getAviFileList(const char *dir_path);
        esp_err_t initDisplayBypass();
        void deinitDisplayBypass();
        esp_err_t initJpegDecoder();
        void deinitJpegDecoder();
        esp_err_t ensureJpegOutputBuffer(int out_len);
        void releaseJpegOutputBuffer();
        bool shouldExitBySwipe();

        static void playTask(void *arg);
        static void videoCallback(frame_data_t *data, void *arg);
        static void audioCallback(frame_data_t *data, void *arg);
        static void audioSetClockCallback(uint32_t rate, uint32_t bits_cfg, uint32_t ch, void *arg);
        static void playEndCallback(void *arg);
    };

} // namespace esp_brookesia::apps
