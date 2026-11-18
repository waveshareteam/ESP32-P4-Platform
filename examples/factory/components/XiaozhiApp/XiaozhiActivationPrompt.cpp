/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XiaozhiActivationPrompt.hpp"
#include "XiaozhiCodec.hpp"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "decoder/impl/esp_opus_dec.h"
#include "third_party/xiaozhi_esp32/ogg_demuxer.h"

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:XiaozhiPrompt"
#include "esp_lib_utils.h"

namespace esp_brookesia::apps {

namespace {

constexpr uint32_t SAMPLE_RATE = 16000;
constexpr uint32_t FRAME_DURATION_MS = 60;
constexpr size_t PCM_MONO_BUFFER_SIZE =
    SAMPLE_RATE * FRAME_DURATION_MS / 1000 * sizeof(int16_t);
constexpr size_t PCM_STEREO_BUFFER_SIZE = PCM_MONO_BUFFER_SIZE * 2;
struct Prompt {
    const uint8_t *begin;
    const uint8_t *end;
};

#define DECLARE_PROMPT(name)                                                                        \
    extern const uint8_t prompt_##name##_begin[] asm("_binary_" #name "_ogg_start");                  \
    extern const uint8_t prompt_##name##_end[] asm("_binary_" #name "_ogg_end")
DECLARE_PROMPT(activation);
DECLARE_PROMPT(success);
DECLARE_PROMPT(0);
DECLARE_PROMPT(1);
DECLARE_PROMPT(2);
DECLARE_PROMPT(3);
DECLARE_PROMPT(4);
DECLARE_PROMPT(5);
DECLARE_PROMPT(6);
DECLARE_PROMPT(7);
DECLARE_PROMPT(8);
DECLARE_PROMPT(9);
#undef DECLARE_PROMPT

#define PROMPT(name) {prompt_##name##_begin, prompt_##name##_end}
const Prompt ACTIVATION_PROMPT = PROMPT(activation);
const Prompt SUCCESS_PROMPT = PROMPT(success);
const Prompt DIGIT_PROMPTS[] = {
    PROMPT(0), PROMPT(1), PROMPT(2), PROMPT(3), PROMPT(4),
    PROMPT(5), PROMPT(6), PROMPT(7), PROMPT(8), PROMPT(9),
};
#undef PROMPT

} // namespace

XiaozhiActivationPrompt::XiaozhiActivationPrompt()
{
    _mutex = xSemaphoreCreateMutex();
}

XiaozhiActivationPrompt::~XiaozhiActivationPrompt()
{
    if (!stopAndWait(3000)) {
        ESP_UTILS_LOGE("Activation prompt task did not stop in time; waiting");
        while (_running.load()) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    if (_mutex) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

bool XiaozhiActivationPrompt::start(const char *activation_code, bool force)
{
    if (!activation_code || !activation_code[0] || !_mutex) {
        return false;
    }

    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    if (_running.load()) {
        xSemaphoreGive(_mutex);
        return false;
    }
    if (!force && strcmp(_announced_code, activation_code) == 0) {
        xSemaphoreGive(_mutex);
        return true;
    }
    snprintf(_pending_code, sizeof(_pending_code), "%s", activation_code);
    _pending_success = false;
    _cancel_requested.store(false);
    _last_playback_completed.store(false);
    _running.store(true);
    xSemaphoreGive(_mutex);

    BaseType_t result = xTaskCreatePinnedToCore(
                            taskEntry,
                            "xiaozhi_activation",
                            TASK_STACK_SIZE,
                            this,
                            5,
                            &_task,
                            0
                        );
    if (result != pdPASS) {
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _task = nullptr;
            xSemaphoreGive(_mutex);
        }
        _running.store(false);
        ESP_UTILS_LOGE("Failed to create activation prompt task");
        return false;
    }
    return true;
}

bool XiaozhiActivationPrompt::playSuccess()
{
    if (!_mutex || xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    if (_running.load()) {
        xSemaphoreGive(_mutex);
        return false;
    }
    _pending_code[0] = '\0';
    _pending_success = true;
    _cancel_requested.store(false);
    _last_playback_completed.store(false);
    _running.store(true);
    xSemaphoreGive(_mutex);

    BaseType_t result = xTaskCreatePinnedToCore(
                            taskEntry,
                            "xiaozhi_success",
                            TASK_STACK_SIZE,
                            this,
                            5,
                            &_task,
                            0
                        );
    if (result != pdPASS) {
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _task = nullptr;
            _pending_success = false;
            xSemaphoreGive(_mutex);
        }
        _running.store(false);
        ESP_UTILS_LOGE("Failed to create success prompt task");
        return false;
    }
    return true;
}

void XiaozhiActivationPrompt::cancel()
{
    if (_mutex && xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _cancel_requested.store(true);
        xSemaphoreGive(_mutex);
    } else {
        _cancel_requested.store(true);
    }
}

bool XiaozhiActivationPrompt::waitUntilIdle(uint32_t timeout_ms) const
{
    TickType_t started = xTaskGetTickCount();
    while (_running.load() &&
           (xTaskGetTickCount() - started) < pdMS_TO_TICKS(timeout_ms)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return !_running.load();
}

bool XiaozhiActivationPrompt::stopAndWait(uint32_t timeout_ms)
{
    cancel();
    return waitUntilIdle(timeout_ms);
}

bool XiaozhiActivationPrompt::isRunning() const
{
    return _running.load();
}

bool XiaozhiActivationPrompt::lastPlaybackCompleted() const
{
    return _last_playback_completed.load();
}

bool XiaozhiActivationPrompt::shouldContinue() const
{
    return !_cancel_requested.load();
}

void XiaozhiActivationPrompt::run()
{
    char activation_code[CODE_CAPACITY] = {};
    bool play_success = false;
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(activation_code, sizeof(activation_code), "%s", _pending_code);
        play_success = _pending_success;
        xSemaphoreGive(_mutex);
    }

    void *decoder = nullptr;
    auto *mono_pcm = static_cast<uint8_t *>(malloc(PCM_MONO_BUFFER_SIZE));
    auto *stereo_pcm = static_cast<int16_t *>(malloc(PCM_STEREO_BUFFER_SIZE));
    auto *demuxer = new (std::nothrow) OggDemuxer();
    bool completed = false;

    esp_opus_dec_cfg_t decoder_config = ESP_OPUS_DEC_CONFIG_DEFAULT();
    decoder_config.sample_rate = SAMPLE_RATE;
    decoder_config.channel = ESP_AUDIO_MONO;
    decoder_config.frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS;
    esp_err_t codec_result = ESP_ERR_INVALID_STATE;

    if ((!play_success && !activation_code[0]) || !shouldContinue()) {
        goto cleanup;
    }

    codec_result = prepareXiaozhiCodec();
    if (!shouldContinue()) {
        goto cleanup;
    }
    if (codec_result != ESP_OK || !mono_pcm || !stereo_pcm || !demuxer ||
            esp_opus_dec_open(&decoder_config, sizeof(decoder_config), &decoder) !=
                ESP_AUDIO_ERR_OK ||
            !decoder) {
        ESP_UTILS_LOGE("Failed to prepare activation prompt audio");
        goto cleanup;
    }

    {
        auto play_prompt = [&](const Prompt &prompt) -> bool {
            if (!shouldContinue()) {
                return false;
            }

            const size_t size = prompt.end - prompt.begin;
            esp_opus_dec_reset(decoder);
            demuxer->Reset();
            bool decode_ok = true;
            size_t decoded_packets = 0;
            demuxer->OnDemuxerFinished(
                [&](const uint8_t *data, int sample_rate, size_t size) {
                    if (!decode_ok || !shouldContinue() ||
                            sample_rate != static_cast<int>(SAMPLE_RATE)) {
                        decode_ok = false;
                        return;
                    }

                    esp_audio_dec_in_raw_t input = {};
                    input.buffer = const_cast<uint8_t *>(data);
                    input.len = size;
                    input.frame_recover = ESP_AUDIO_DEC_RECOVERY_NONE;
                    esp_audio_dec_out_frame_t output = {};
                    output.buffer = mono_pcm;
                    output.len = PCM_MONO_BUFFER_SIZE;
                    esp_audio_dec_info_t info = {};
                    if (esp_opus_dec_decode(decoder, &input, &output, &info) !=
                            ESP_AUDIO_ERR_OK ||
                            output.decoded_size == 0 ||
                            output.decoded_size > PCM_MONO_BUFFER_SIZE) {
                        decode_ok = false;
                        return;
                    }

                    size_t sample_count = output.decoded_size / sizeof(int16_t);
                    auto *mono_samples = reinterpret_cast<int16_t *>(mono_pcm);
                    for (size_t i = 0; i < sample_count; ++i) {
                        stereo_pcm[i * 2] = mono_samples[i];
                        stereo_pcm[i * 2 + 1] = mono_samples[i];
                    }

                    size_t stereo_size = sample_count * 2 * sizeof(int16_t);
                    size_t bytes_written = 0;
                    if (!shouldContinue() ||
                            product_i2s_write(stereo_pcm, stereo_size, &bytes_written, 100) != ESP_OK ||
                            bytes_written != stereo_size) {
                        decode_ok = false;
                        return;
                    }
                    ++decoded_packets;
                }
            );
            const size_t processed = demuxer->Process(prompt.begin, size);
            return decode_ok && decoded_packets > 0 && processed == size && shouldContinue();
        };

        if (play_success) {
            completed = play_prompt(SUCCESS_PROMPT);
        } else {
            completed = play_prompt(ACTIVATION_PROMPT);
            for (const char *digit = activation_code;
                    completed && *digit && shouldContinue();
                    ++digit) {
                if (*digit < '0' || *digit > '9') {
                    continue;
                }
                completed = play_prompt(DIGIT_PROMPTS[*digit - '0']);
            }
        }
    }

cleanup:
    if (decoder) {
        esp_opus_dec_close(decoder);
    }
    delete demuxer;
    free(stereo_pcm);
    free(mono_pcm);

    bool playback_completed = completed && shouldContinue();
    _last_playback_completed.store(playback_completed);
    if (!play_success && playback_completed && _mutex &&
            xSemaphoreTake(_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(_announced_code, sizeof(_announced_code), "%s", activation_code);
        xSemaphoreGive(_mutex);
    }

    ESP_UTILS_LOGI(
        "Prompt finished (kind=%s, complete=%d, min_stack_free=%u)",
        play_success ? "success" : "activation",
        playback_completed,
        static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr))
    );
    if (_mutex && xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _task = nullptr;
        _pending_success = false;
        xSemaphoreGive(_mutex);
    } else {
        _task = nullptr;
    }
    _running.store(false);
    vTaskDelete(nullptr);
}

void XiaozhiActivationPrompt::taskEntry(void *arg)
{
    static_cast<XiaozhiActivationPrompt *>(arg)->run();
}

} // namespace esp_brookesia::apps
