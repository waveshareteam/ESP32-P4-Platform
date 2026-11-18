/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_codec_dev_defaults.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"

#include "bsp/esp-bsp.h"
#include "product_support.h"

static const char *TAG = "product_support";

static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;

static bool _is_audio_init = false;
static bool _is_player_init = false;
static int _vloume_intensity = PRODUCT_CODEC_DEFAULT_VOLUME;

static audio_player_cb_t audio_idle_callback = NULL;
static void *audio_idle_cb_user_data = NULL;
static char audio_file_path[128];

#define PRODUCT_BRIGHTNESS_I2C_ADDR    (0x45)
#define PRODUCT_BRIGHTNESS_REG         (0x96)
#define PRODUCT_BRIGHTNESS_DEFAULT     (100)

/**************************************************************************************************
 *
 * Extra Board Function
 *
 **************************************************************************************************/

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    // Volume saved when muting and restored when unmuting. Restoring volume is necessary
    // as es8311_set_voice_mute(true) results in voice volume (REG32) being set to zero.

    product_codec_mute_set(setting == AUDIO_PLAYER_MUTE ? true : false);

    // restore the voice volume upon unmuting
    if (setting == AUDIO_PLAYER_UNMUTE) {
        ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(play_dev_handle, _vloume_intensity), TAG, "Set Codec volume failed");
    }

    return ESP_OK;
}

static void audio_callback(audio_player_cb_ctx_t *ctx)
{
    if (audio_idle_callback) {
        ctx->user_ctx = audio_idle_cb_user_data;
        audio_idle_callback(ctx);
    }
}

int product_display_brightness_get(void)
{
    uint8_t reg_addr = PRODUCT_BRIGHTNESS_REG;
    uint8_t data = 0;
    i2c_master_dev_handle_t dev_handle = NULL;
    i2c_master_bus_handle_t bus_handle = NULL;
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = 100 * 1000,
        .device_address = PRODUCT_BRIGHTNESS_I2C_ADDR,
    };

    esp_err_t ret = bsp_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "brightness get: init i2c failed, %s", esp_err_to_name(ret));
        return PRODUCT_BRIGHTNESS_DEFAULT;
    }

    bus_handle = bsp_i2c_get_handle();
    if (bus_handle == NULL) {
        ESP_LOGW(TAG, "brightness get: i2c bus handle is NULL");
        return PRODUCT_BRIGHTNESS_DEFAULT;
    }

    ret = i2c_master_bus_add_device(bus_handle, &i2c_dev_conf, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "brightness get: add i2c device failed, %s", esp_err_to_name(ret));
        return PRODUCT_BRIGHTNESS_DEFAULT;
    }

    ret = i2c_master_transmit_receive(dev_handle, &reg_addr, sizeof(reg_addr), &data, sizeof(data), 50);
    i2c_master_bus_rm_device(dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "brightness get: read i2c register failed, %s", esp_err_to_name(ret));
        return PRODUCT_BRIGHTNESS_DEFAULT;
    }

    return (data * 100 + 127) / 255;
}

esp_err_t product_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    *bytes_read = len;
    return ret;
}

esp_err_t product_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

esp_err_t product_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
    }
    if (record_dev_handle) {
        ret |= esp_codec_dev_close(record_dev_handle);
    }

    if (play_dev_handle) {
        ret |= esp_codec_dev_open(play_dev_handle, &fs);
    }
    if (record_dev_handle) {
        esp_err_t record_status = esp_codec_dev_open(record_dev_handle, &fs);
        if (record_status == ESP_OK) {
            record_status = esp_codec_dev_set_in_gain(record_dev_handle, PRODUCT_CODEC_DEFAULT_ADC_VOLUME);
            if (record_status == ESP_OK) {
                record_status = product_codec_mic_boost_set(PRODUCT_CODEC_MIC_BOOST_DB);
            }
        }
        if (ret == ESP_OK) ret = record_status;
    }
    return ret;
}

esp_err_t product_codec_volume_set(int volume, int *volume_set)
{
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(play_dev_handle, volume), TAG, "Set Codec volume failed");
    _vloume_intensity = volume;

    ESP_LOGI(TAG, "Setting volume: %d", volume);

    return ESP_OK;
}

int product_codec_volume_get(void)
{
    return _vloume_intensity;
}

esp_err_t product_codec_mic_boost_set(int gain_db)
{
    // ES8311 ADC_VOLUME (0x17) uses 0xBF for unity and 0.5 dB per step.
    const int adc_volume_reg = 0x17;
    const int adc_volume_unity = 0xBF;
    ESP_RETURN_ON_FALSE(record_dev_handle, ESP_ERR_INVALID_STATE, TAG, "Recording codec unavailable");
    ESP_RETURN_ON_FALSE(gain_db >= 0 && gain_db <= 32, ESP_ERR_INVALID_ARG, TAG, "Invalid microphone boost");
    return esp_codec_dev_write_reg(record_dev_handle, adc_volume_reg, adc_volume_unity + 2 * gain_db);
}

esp_err_t product_codec_mute_set(bool enable)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    return ret;
}

esp_err_t product_codec_dev_stop(void)
{
    esp_err_t ret = ESP_OK;

    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
    }

    if (record_dev_handle) {
        ret = esp_codec_dev_close(record_dev_handle);
    }
    return ret;
}

esp_err_t product_codec_dev_resume(void)
{
    return product_codec_set_fs(PRODUCT_CODEC_DEFAULT_SAMPLE_RATE, PRODUCT_CODEC_DEFAULT_BIT_WIDTH, PRODUCT_CODEC_DEFAULT_CHANNEL);
}

esp_err_t product_codec_init(void)
{
    if (_is_audio_init) {
        return ESP_OK;
    }

    play_dev_handle = bsp_audio_codec_speaker_init();
    assert((play_dev_handle) && "play_dev_handle not initialized");

    record_dev_handle = bsp_audio_codec_microphone_init();
    assert((record_dev_handle) && "record_dev_handle not initialized");

    ESP_RETURN_ON_ERROR(product_codec_dev_resume(), TAG, "Configure audio codec failed");

    _is_audio_init = true;

    return ESP_OK;
}

esp_err_t product_player_init(void)
{
    if (_is_player_init) {
        return ESP_OK;
    }

    audio_player_config_t config = { .mute_fn = audio_mute_function,
                                     .write_fn = product_i2s_write,
                                     .clk_set_fn = product_codec_set_fs,
                                     .priority = 5
                                   };
    ESP_RETURN_ON_ERROR(audio_player_new(config), TAG, "audio_player_init failed");
    audio_player_callback_register(audio_callback, NULL);

    _is_player_init = true;

    return ESP_OK;
}

esp_err_t product_player_del(void)
{
    _is_player_init = false;

    ESP_RETURN_ON_ERROR(audio_player_delete(), TAG, "audio_player_delete failed");

    return ESP_OK;
}

esp_err_t product_file_instance_init(const char *path, file_iterator_instance_t **ret_instance)
{
    ESP_RETURN_ON_FALSE(path, ESP_FAIL, TAG, "path is NULL");
    ESP_RETURN_ON_FALSE(ret_instance, ESP_FAIL, TAG, "ret_instance is NULL");

    file_iterator_instance_t *file_iterator = file_iterator_new(path);
    ESP_RETURN_ON_FALSE(file_iterator, ESP_FAIL, TAG, "file_iterator_new failed, %s", path);

    *ret_instance = file_iterator;

    return ESP_OK;
}

void product_file_instance_del(file_iterator_instance_t *instance)
{
    if (!instance) {
        return;
    }

    // esp-file-iterator 1.0.0 declares file_iterator_delete but does not define
    // it. Its constructor owns each list entry, the list and a duplicated path.
    for (size_t index = 0; index < instance->count; ++index) {
        free(instance->list[index]);
    }
    free(instance->list);
    free((void *)instance->directory_path);
    free(instance);
}

esp_err_t product_player_play_index(file_iterator_instance_t *instance, int index)
{
    ESP_RETURN_ON_FALSE(instance, ESP_FAIL, TAG, "instance is NULL");

    ESP_LOGI(TAG, "play_index(%d)", index);
    char filename[128];
    int retval = file_iterator_get_full_path_from_index(instance, index, filename, sizeof(filename));
    ESP_RETURN_ON_FALSE(retval != 0, ESP_FAIL, TAG, "file_iterator_get_full_path_from_index failed");

    ESP_LOGI(TAG, "opening file '%s'", filename);
    FILE *fp = fopen(filename, "rb");
    ESP_RETURN_ON_FALSE(fp, ESP_FAIL, TAG, "unable to open file");

    ESP_LOGI(TAG, "Playing '%s'", filename);
    ESP_RETURN_ON_ERROR(audio_player_play(fp), TAG, "audio_player_play failed");

    memcpy(audio_file_path, filename, sizeof(audio_file_path));

    return ESP_OK;
}

esp_err_t product_player_play_file(const char *file_path)
{
    ESP_LOGI(TAG, "opening file '%s'", file_path);
    FILE *fp = fopen(file_path, "rb");
    ESP_RETURN_ON_FALSE(fp, ESP_FAIL, TAG, "unable to open file");

    ESP_LOGI(TAG, "Playing '%s'", file_path);
    ESP_RETURN_ON_ERROR(audio_player_play(fp), TAG, "audio_player_play failed");

    memcpy(audio_file_path, file_path, sizeof(audio_file_path));

    return ESP_OK;
}

void product_player_register_callback(audio_player_cb_t cb, void *user_data)
{
    audio_idle_callback = cb;
    audio_idle_cb_user_data = user_data;
}

bool product_player_is_playing_by_path(const char *file_path)
{
    return (strcmp(audio_file_path, file_path) == 0);
}

bool product_player_is_playing_by_index(file_iterator_instance_t *instance, int index)
{
    return (index == file_iterator_get_index(instance));
}
