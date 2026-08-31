/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_check.h"
#include "example_config.h"

#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
#include "bsp/esp-bsp.h"
#include "esp_codec_dev.h"
#else
#include "es8311.h"
#endif

static const char *TAG = "i2s_es8311";
static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"
                                     };
#if !(CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4)
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
#else
static esp_codec_dev_handle_t s_play_dev;
static esp_codec_dev_handle_t s_record_dev;
static bool s_play_opened;
static bool s_record_opened;
#endif

/* Import music file as buffer */
#if CONFIG_EXAMPLE_MODE_MUSIC
extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");
#endif

static void gpio_init(void)
{
#if !CONFIG_EXAMPLE_BSP
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_PA),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_PA, 1);
#endif
}

#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
static esp_err_t p4_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    ESP_RETURN_ON_FALSE(s_play_dev || s_record_dev, ESP_ERR_INVALID_STATE, TAG, "codec is not initialized");

    esp_err_t ret = ESP_OK;
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    if (s_play_opened) {
        ret |= esp_codec_dev_close(s_play_dev);
        s_play_opened = false;
    }
    if (s_record_opened) {
        ret |= esp_codec_dev_close(s_record_dev);
        s_record_opened = false;
    }
    if (s_play_dev) {
        ret |= esp_codec_dev_open(s_play_dev, &fs);
        if (ret == ESP_OK) {
            s_play_opened = true;
        }
    }
    if (s_record_dev) {
        ret |= esp_codec_dev_set_in_gain(s_record_dev, 24.0);
        ret |= esp_codec_dev_open(s_record_dev, &fs);
        if (ret == ESP_OK) {
            s_record_opened = true;
        }
    }
    return ret;
}

static esp_err_t p4_codec_volume_set(int volume)
{
    ESP_RETURN_ON_FALSE(s_play_dev, ESP_ERR_INVALID_STATE, TAG, "speaker codec is not initialized");
    return esp_codec_dev_set_out_vol(s_play_dev, volume);
}
#endif

static esp_err_t es8311_codec_init(void)
{
#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
    s_play_dev = bsp_audio_codec_speaker_init();
    ESP_RETURN_ON_FALSE(s_play_dev, ESP_FAIL, TAG, "init ESP32-P4 speaker codec failed");
    s_record_dev = bsp_audio_codec_microphone_init();
    ESP_RETURN_ON_FALSE(s_record_dev, ESP_FAIL, TAG, "init ESP32-P4 microphone codec failed");
    ESP_RETURN_ON_ERROR(p4_codec_set_fs(EXAMPLE_SAMPLE_RATE, 16, I2S_SLOT_MODE_STEREO), TAG, "set ESP32-P4 BSP codec format failed");
    ESP_RETURN_ON_ERROR(p4_codec_volume_set(EXAMPLE_VOICE_VOLUME), TAG, "set ESP32-P4 BSP codec volume failed");
    return ESP_OK;
#else
#if !CONFIG_EXAMPLE_BSP
    const i2c_config_t es_i2c_cfg = {
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "config i2c failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0), TAG, "install i2c driver failed");
#else
    ESP_ERROR_CHECK(bsp_i2c_init());
#endif

    es8311_handle_t es_handle = es8311_create(I2C_NUM, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
        .sample_frequency = EXAMPLE_SAMPLE_RATE
    };

    ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE, EXAMPLE_SAMPLE_RATE), TAG, "set es8311 sample frequency failed");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");
#if CONFIG_EXAMPLE_MODE_ECHO
    ESP_RETURN_ON_ERROR(es8311_microphone_gain_set(es_handle, EXAMPLE_MIC_GAIN), TAG, "set es8311 microphone gain failed");
#endif
    return ESP_OK;
#endif
}

static esp_err_t i2s_driver_init(void)
{
#if !CONFIG_EXAMPLE_BSP
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle));
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
#elif CONFIG_IDF_TARGET_ESP32P4
    ESP_LOGI(TAG, "Using ESP32-P4 BSP for I2S and codec configuration");
#else
    ESP_LOGI(TAG, "Using BSP for HW configuration");
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;
    ESP_ERROR_CHECK(bsp_audio_init(&std_cfg, &tx_handle, &rx_handle));
    ESP_ERROR_CHECK(bsp_audio_poweramp_enable(true));
#endif
    return ESP_OK;
}

#if CONFIG_EXAMPLE_MODE_MUSIC
static void i2s_music(void *args)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;
    uint8_t *data_ptr = (uint8_t *)music_pcm_start;
    const size_t music_size = music_pcm_end - music_pcm_start;

#if !(CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4)
    ESP_ERROR_CHECK(i2s_channel_disable(tx_handle));
    ESP_ERROR_CHECK(i2s_channel_preload_data(tx_handle, data_ptr, music_size, &bytes_write));
    data_ptr += bytes_write;
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
#endif

    while (1) {
#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
        data_ptr = (uint8_t *)music_pcm_start;
        ret = esp_codec_dev_write(s_play_dev, data_ptr, music_size);
        bytes_write = (ret == ESP_OK) ? music_size : 0;
#else
        ret = i2s_channel_write(tx_handle, data_ptr, music_pcm_end - data_ptr, &bytes_write, portMAX_DELAY);
#endif
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[music] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_write > 0) {
            ESP_LOGI(TAG, "[music] i2s music played, %u bytes are written.", (unsigned int)bytes_write);
        } else {
            ESP_LOGE(TAG, "[music] i2s music play failed.");
            abort();
        }
        data_ptr = (uint8_t *)music_pcm_start;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

#else
static void i2s_echo(void *args)
{
    int *mic_data = malloc(EXAMPLE_RECV_BUF_SIZE);
    if (!mic_data) {
        ESP_LOGE(TAG, "[echo] No memory for read data buffer");
        abort();
    }
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_write = 0;
    ESP_LOGI(TAG, "[echo] Echo start");

    while (1) {
        memset(mic_data, 0, EXAMPLE_RECV_BUF_SIZE);
#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
        ret = esp_codec_dev_read(s_record_dev, mic_data, EXAMPLE_RECV_BUF_SIZE);
        bytes_read = (ret == ESP_OK) ? EXAMPLE_RECV_BUF_SIZE : 0;
#else
        ret = i2s_channel_read(rx_handle, mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_read, 1000);
#endif
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s read failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
#if CONFIG_EXAMPLE_BSP && CONFIG_IDF_TARGET_ESP32P4
        ret = esp_codec_dev_write(s_play_dev, mic_data, EXAMPLE_RECV_BUF_SIZE);
        bytes_write = (ret == ESP_OK) ? EXAMPLE_RECV_BUF_SIZE : 0;
#else
        ret = i2s_channel_write(tx_handle, mic_data, EXAMPLE_RECV_BUF_SIZE, &bytes_write, 1000);
#endif
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[echo] i2s write failed, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }
        if (bytes_read != bytes_write) {
            ESP_LOGW(TAG, "[echo] %u bytes read but only %u bytes are written", (unsigned int)bytes_read, (unsigned int)bytes_write);
        }
    }
    vTaskDelete(NULL);
}
#endif

void app_main(void)
{
    gpio_init();
    printf("i2s es8311 codec example start\n-----------------------------\n");
    if (i2s_driver_init() != ESP_OK) {
        ESP_LOGE(TAG, "i2s driver init failed");
        abort();
    } else {
        ESP_LOGI(TAG, "i2s driver init success");
    }

    if (es8311_codec_init() != ESP_OK) {
        ESP_LOGE(TAG, "es8311 codec init failed");
        abort();
    } else {
        ESP_LOGI(TAG, "es8311 codec init success");
    }
#if CONFIG_EXAMPLE_MODE_MUSIC
    xTaskCreate(i2s_music, "i2s_music", 4096, NULL, 5, NULL);
#else
    xTaskCreate(i2s_echo, "i2s_echo", 8192, NULL, 5, NULL);
#endif
}
