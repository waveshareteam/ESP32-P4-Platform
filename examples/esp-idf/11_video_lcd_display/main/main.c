/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include "esp_err.h"
#include "esp_log.h"
#include "esp_video_init.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_private/esp_cache_private.h"
#include "esp_timer.h"
#include "driver/ppa.h"
#include "app_video.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"

#include "bsp/display.h"
#include "bsp/esp-bsp.h"

#include "lvgl.h"
#include "lv_demos.h"

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

static void camera_video_frame_operation(
    uint8_t *camera_buf,
    uint8_t camera_buf_index,
    uint32_t camera_buf_hes,
    uint32_t camera_buf_ves,
    size_t camera_buf_len,
    void *user_data);

static const char *TAG = "app_main";

static esp_lcd_panel_handle_t display_panel;
static ppa_client_handle_t ppa_srm_handle = NULL;
static size_t data_cache_line_size = 0;
static void *lcd_buffer[CONFIG_BSP_LCD_DPI_BUFFER_NUMS];
static lv_display_t *disp;

i2c_master_bus_handle_t i2c_bus_;

static bool dummy_draw_enabled = true;
static bool dummy_mode_delay_flag = false;

static void calc_ppa_input_offset(uint32_t src_w, uint32_t src_h,
                                  uint32_t dst_w, uint32_t dst_h,
                                  uint32_t *offset_x, uint32_t *offset_y)
{
    *offset_x = (src_w > dst_w) ? (src_w - dst_w) / 2 : 0;
    *offset_y = (src_h > dst_h) ? (src_h - dst_h) / 2 : 0;

    if ((*offset_x + dst_w) > src_w)
    {
        *offset_x = 0;
    }
    if ((*offset_y + dst_h) > src_h)
    {
        *offset_y = 0;
    }
}

void app_main(void)
{
    disp = bsp_display_start();
    bsp_display_backlight_on();

    ESP_ERROR_CHECK(esp_lv_adapter_set_dummy_draw(disp, dummy_draw_enabled));

    display_panel = bsp_display_get_panel_handle();

    ppa_client_config_t ppa_srm_config = {
        .oper_type = PPA_OPERATION_SRM,
    };
    ESP_ERROR_CHECK(ppa_register_client(&ppa_srm_config, &ppa_srm_handle));

    ESP_ERROR_CHECK(esp_cache_get_alignment(MALLOC_CAP_SPIRAM, &data_cache_line_size));

    i2c_bus_ = bsp_i2c_get_handle();

    esp_err_t ret = app_video_main(i2c_bus_);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "video main init failed with err=0x%x", ret);
        return;
    }

    int video_cam_fd0 = app_video_open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, APP_VIDEO_FMT);
    if (video_cam_fd0 < 0)
    {
        ESP_LOGE(TAG, "video cam open failed");
        return;
    }

    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(
        display_panel, 3,
        &lcd_buffer[0], &lcd_buffer[1], &lcd_buffer[2]));

    ESP_LOGI(TAG, "Using user defined buffer");
    void *camera_buf[2];
    for (int i = 0; i < 2; i++)
    {
        camera_buf[i] = heap_caps_aligned_calloc(
            data_cache_line_size,
            1,
            app_video_get_buf_size(),
            MALLOC_CAP_SPIRAM);
    }
    ESP_ERROR_CHECK(app_video_set_bufs(video_cam_fd0, 2, (void *)camera_buf));

    ESP_ERROR_CHECK(app_video_register_frame_operation_cb(camera_video_frame_operation));

    ESP_ERROR_CHECK(app_video_stream_task_start(video_cam_fd0, 0, NULL));

}

static void camera_video_frame_operation(
    uint8_t *camera_buf,
    uint8_t camera_buf_index,
    uint32_t camera_buf_hes,
    uint32_t camera_buf_ves,
    size_t camera_buf_len,
    void *user_data)
{

    if (!dummy_draw_enabled || dummy_mode_delay_flag)
    {
        return;
    }

    const uint32_t display_width = BSP_LCD_H_RES;
    const uint32_t display_height = BSP_LCD_V_RES;

    if (display_height == 0)
    {
        ESP_LOGE(TAG, "Display height is zero!");
        return;
    }

    uint32_t in_offset_x = 0;
    uint32_t in_offset_y = 0;
    calc_ppa_input_offset(camera_buf_hes, camera_buf_ves,
                          display_width, display_height,
                          &in_offset_x, &in_offset_y);

    uint32_t in_block_w = (camera_buf_hes > display_width) ? display_width : camera_buf_hes;
    uint32_t in_block_h = (camera_buf_ves > display_height) ? display_height : camera_buf_ves;

    ppa_srm_oper_config_t srm_config = {
        .in.buffer = camera_buf,
        .in.pic_w = camera_buf_hes,
        .in.pic_h = camera_buf_ves,
        .in.block_w = in_block_w,
        .in.block_h = in_block_h,
        .in.block_offset_x = in_offset_x,
        .in.block_offset_y = in_offset_y,
        .in.srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,

        .out.buffer = lcd_buffer[camera_buf_index],
        .out.buffer_size = ALIGN_UP(display_width * display_height *
                                        (APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? 2 : 3),
                                    data_cache_line_size),
        .out.pic_w = display_width,
        .out.pic_h = display_height,
        .out.block_offset_x = 0,
        .out.block_offset_y = 0,
        .out.srm_cm = APP_VIDEO_FMT == APP_VIDEO_FMT_RGB565 ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888,

        .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
        .scale_x = 1,
        .scale_y = 1,
        .mirror_x = 0,
        .mirror_y = 0,
        .rgb_swap = 0,
        .byte_swap = 0,
        .mode = PPA_TRANS_MODE_BLOCKING,
    };

    esp_err_t ret = ppa_do_scale_rotate_mirror(ppa_srm_handle, &srm_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "PPA SRM failed: %d", ret);
        return;
    }

    uint32_t draw_w = (camera_buf_hes > display_width) ? display_width : camera_buf_hes;
    uint32_t draw_h = (camera_buf_ves > display_height) ? display_height : camera_buf_ves;

    if (dummy_draw_enabled && !dummy_mode_delay_flag)
    {
        ret = esp_lv_adapter_dummy_draw_blit(
            disp,
            0, 0,
            draw_w,
            draw_h,
            lcd_buffer[camera_buf_index],
            true);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Dummy draw blit failed: %d", ret);
        }
    }
}