#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "esp_task.h"
#include "esp_heap_caps.h"

#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:SpecAnalyzer"
#include "esp_lib_utils.h"
#include "SpecAnalyzer.hpp"
#include <cmath>
#define APP_NAME "SpecAnalyzer"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

LV_IMG_DECLARE(img_app_specanalyzer);

namespace esp_brookesia::apps {

SpecAnalyzer *SpecAnalyzer::_instance = nullptr;

SpecAnalyzer *SpecAnalyzer::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new SpecAnalyzer(use_status_bar, use_navigation_bar);
        // The smoothing buffer belongs to the singleton instance and must be reset
        // after allocation, before any timer callback can read it.
        memset(&(_instance->_smooth_spectrum[0][0]), 0, sizeof(_instance->_smooth_spectrum));
    }
    return _instance;
}

SpecAnalyzer::SpecAnalyzer(bool use_status_bar, bool use_navigation_bar) : 
    App(APP_NAME, &img_app_specanalyzer, true, use_status_bar, use_navigation_bar),
    _canvas(nullptr),
    _mic_label(nullptr),
    _timer(nullptr),
    _audio_task_handle(nullptr),
    _audio_task_running(false),
    _draw_buf(nullptr)
{
    // Clear DSP state so the first rendered frame starts from silence.
    memset(_raw_data, 0, sizeof(_raw_data));
    memset(_audio_buffer, 0, sizeof(_audio_buffer));
    memset(_fft_buffer, 0, sizeof(_fft_buffer));
    memset(_spectrum, 0, sizeof(_spectrum));
    memset(_display_spectrum, 0, sizeof(_display_spectrum));
    memset(_peak, 0, sizeof(_peak));
    memset(_smooth_spectrum, 0, sizeof(_smooth_spectrum));
}

SpecAnalyzer::~SpecAnalyzer()
{
    close();
}

bool SpecAnalyzer::run(void)
{
    ESP_UTILS_LOGD("Run");

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // Prefer PSRAM for the large canvas buffer and fall back to SRAM if PSRAM is exhausted.
    size_t buf_size = CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t);
    _draw_buf = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    
    if (!_draw_buf) {
        ESP_UTILS_LOGE("PSRAM allocation failed! Using fallback SRAM");
        _draw_buf = (lv_color_t*)malloc(buf_size);
    }
    
    if (!_draw_buf) {
        ESP_UTILS_LOGE("Canvas buffer allocation failed!");
        return false;
    }
    
    // Draw into an LVGL canvas so the FFT task only updates numeric buffers and the
    // GUI timer owns all rendering work on the LVGL thread.
    _canvas = lv_canvas_create(lv_scr_act());
    if (!_canvas) {
        ESP_UTILS_LOGE("Canvas creation failed!");
        free(_draw_buf);
        _draw_buf = nullptr;
        return false;
    }
    lv_obj_set_size(_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_align(_canvas, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_set_buffer(_canvas, _draw_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565);
    
    // Initialize the ESP-DSP radix-2 FFT tables once before the audio task starts.
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_UTILS_LOGE("FFT init failed: %d", ret);
        close();
        return false;
    }
    
    // Hann window reduces spectral leakage before the FFT.
    dsps_wind_hann_f32(_wind, N_SAMPLES);
    ESP_UTILS_LOGI("FFT and window initialized");
    
    // Refresh at about 30 FPS; the values are already smoothed by the DSP task.
    _timer = lv_timer_create(timer_cb, 33, this);
    if (!_timer) {
        ESP_UTILS_LOGE("Spectrum timer creation failed!");
        close();
        return false;
    }
    
    // The I2S read can block, so it runs outside the LVGL task and publishes smoothed bins.
    _audio_task_running = true;
    BaseType_t task_ret = xTaskCreatePinnedToCore(
        audio_fft_task, 
        "audio_fft", 
        8 * 1024, 
        this, 
        5, 
        &_audio_task_handle, 
        PRO_CPU_NUM
    );
    if (task_ret != pdPASS) {
        ESP_UTILS_LOGE("Audio FFT task creation failed!");
        _audio_task_running = false;
        close();
        return false;
    }
    
    _mic_label = lv_label_create(scr);
    lv_label_set_text(_mic_label, "Mic");
    lv_obj_set_style_text_color(_mic_label, lv_color_hex(0x00BFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(_mic_label, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_mic_label, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_mic_label, 6, LV_PART_MAIN);
    lv_obj_align_to(_mic_label, _canvas, LV_ALIGN_TOP_LEFT, 20, 12);
    
    return true;
}

bool SpecAnalyzer::back(void)
{
    ESP_UTILS_LOGD("Back");
    notifyCoreClosed();
    return true;
}

bool SpecAnalyzer::close(void)
{
    ESP_UTILS_LOGD("Close");
    
    if (_audio_task_running) {
        _audio_task_running = false;
        
        for (int i = 0; i < 20; i++) {
            if (_audio_task_handle == nullptr) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        if (_audio_task_handle) {
            vTaskDelete(_audio_task_handle);
            _audio_task_handle = nullptr;
        }
    }
    
    if (_timer) {
        lv_timer_del(_timer);
        _timer = nullptr;
    }
    
    if (_canvas) {
        lv_obj_del(_canvas);
        _canvas = nullptr;
    }

    if (_mic_label) {
        lv_obj_del(_mic_label);
        _mic_label = nullptr;
    }
    
    if (_draw_buf) {
        free(_draw_buf);
        _draw_buf = nullptr;
    }
    
    return true;
}

bool SpecAnalyzer::init() { return true; }
bool SpecAnalyzer::deinit() { return true; }
bool SpecAnalyzer::pause() { return true; }
bool SpecAnalyzer::resume() { return true; }

// Audio processing task. The current hardware renders one microphone while the BSP
// frame layout may still contain interleaved stereo samples.
void SpecAnalyzer::audio_fft_task(void *pvParameters)
{
    SpecAnalyzer *app = static_cast<SpecAnalyzer*>(pvParameters);
    size_t bytes_read;
    
    while (app->_audio_task_running) {
        esp_err_t ret = product_i2s_read(
            app->_raw_data, 
            N_SAMPLES * I2S_CHANNELS * sizeof(int16_t), 
            &bytes_read, 
            portMAX_DELAY
        );
        
        if (ret != ESP_OK || bytes_read < N_SAMPLES * sizeof(int16_t)) {
            ESP_UTILS_LOGW("I2S read error: %d, bytes: %d", ret, bytes_read);
            continue;
        }
        
        const size_t samples_read = bytes_read / sizeof(int16_t);
        const bool has_interleaved_channels = samples_read >= (N_SAMPLES * I2S_CHANNELS);
        for (int i = 0; i < N_SAMPLES; i++) {
            size_t sample_index = has_interleaved_channels ? (i * I2S_CHANNELS) : i;
            app->_audio_buffer[0][i] = app->_raw_data[sample_index] / 32768.0f;
        }
        
        for (int ch = 0; ch < MIC_COUNT; ch++) {
            // Apply the window in place before constructing the complex FFT input.
            dsps_mul_f32(app->_audio_buffer[ch], app->_wind, app->_audio_buffer[ch], N_SAMPLES, 1, 1, 1);
            
            for (int i = 0; i < N_SAMPLES; i++) {
                app->_fft_buffer[ch][2 * i] = app->_audio_buffer[ch][i];
                app->_fft_buffer[ch][2 * i + 1] = 0;
            }
            
            dsps_fft2r_fc32(app->_fft_buffer[ch], N_SAMPLES);
            dsps_bit_rev_fc32(app->_fft_buffer[ch], N_SAMPLES);
            
            // Convert complex bins to dB. The floor term avoids log10(0).
            for (int i = 0; i < N_SAMPLES / 2; i++) {
                float real = app->_fft_buffer[ch][2 * i];
                float imag = app->_fft_buffer[ch][2 * i + 1];
                float magnitude = sqrtf(real * real + imag * imag);
                app->_spectrum[ch][i] = 20 * log10f(magnitude / (N_SAMPLES / 2) + 1e-9);
            }
            
            // Map FFT bins onto fewer visual bars and smooth the change over time.
            for (int i = 0; i < STRIPE_COUNT; i++) {
                int fft_idx = i * (N_SAMPLES / 2) / STRIPE_COUNT;
                float boosted_db = app->_spectrum[ch][fft_idx] + 10.0f;
                app->_display_spectrum[ch][i] = fmaxf(-90.0f, fminf(0.0f, boosted_db));
                
                const float SMOOTH_FACTOR = 0.2f;
                app->_smooth_spectrum[ch][i] = app->_smooth_spectrum[ch][i] * (1.0f - SMOOTH_FACTOR) + 
                                              app->_display_spectrum[ch][i] * SMOOTH_FACTOR;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    app->_audio_task_handle = nullptr;
    vTaskDelete(NULL);
}

// LVGL timer callback. It renders the smoothed FFT bins and peak markers into the canvas.
void SpecAnalyzer::timer_cb(lv_timer_t *timer)
{
    SpecAnalyzer *app = static_cast<SpecAnalyzer*>(lv_timer_get_user_data(timer));
    if (!app || !app->_canvas) return;
    
    lv_layer_t layer;
    lv_canvas_init_layer(app->_canvas, &layer);
    lv_canvas_fill_bg(app->_canvas, lv_color_black(), LV_OPA_COVER);
    
    const int bar_total_width = app->CANVAS_WIDTH - 40;          // Leave side margins
    const int bar_width = bar_total_width / app->STRIPE_COUNT - 2;
    const int bar_gap = 2;
    const int base_y = app->CANVAS_HEIGHT - 20;                  // Bottom margin
    const int max_bar_height = app->CANVAS_HEIGHT - 50;
    const int BASE_BAR_HEIGHT = 5;
    
    for (int ch = 0; ch < app->MIC_COUNT; ch++) {
        int channel_offset_x = 20;
        
        for (int i = 0; i < app->STRIPE_COUNT; i++) {
            // Normalize smoothed dB values from [-90, 0] to [0, 1].
            float db = app->_smooth_spectrum[ch][i];
            float norm = (db + 90.0f) / 90.0f;
            norm = fmaxf(0.0f, fminf(1.0f, norm));
            
            // Nonlinear mapping keeps quiet audio visible while preserving peaks.
            int bar_height = (int)(powf(norm, 0.8) * max_bar_height);
            if (bar_height < BASE_BAR_HEIGHT) {
                bar_height = BASE_BAR_HEIGHT;
            }
            
            // Peak markers rise immediately and fall faster when they are higher,
            // which gives the analyzer a natural decay without another timer.
            if (app->_peak[ch][i] < bar_height) {
                app->_peak[ch][i] = bar_height;
            } else {
                float peak_norm = app->_peak[ch][i] / (float)max_bar_height;
                float fall_speed = 0.16f + (peak_norm * 0.4f);
                app->_peak[ch][i] -= fall_speed;
                
                if (app->_peak[ch][i] < BASE_BAR_HEIGHT) {
                    app->_peak[ch][i] = BASE_BAR_HEIGHT;
                }
            }
            
            int bar_x1 = channel_offset_x + (i * (bar_width + bar_gap));
            int bar_x2 = bar_x1 + bar_width;
            int bar_y1 = base_y - bar_height;
            int bar_y2 = base_y;
            
            // Assign a slightly different hue per bin so adjacent bars remain readable.
            float hue_base = 190.0f;
            float hue_step = 140.0f / app->STRIPE_COUNT;
            uint16_t hue = (uint16_t)(hue_base + i * hue_step);
            lv_color_t main_color = lv_color_hsv_to_rgb(hue, 100, 100);
            lv_color_t light_color = lv_color_hsv_to_rgb(hue, 80, 100);
            lv_color_t dark_color = lv_color_hsv_to_rgb(hue, 100, 70);
            
            lv_draw_rect_dsc_t bar_dsc;
            lv_draw_rect_dsc_init(&bar_dsc);
            bar_dsc.bg_opa = LV_OPA_COVER;
            
            bar_dsc.bg_color = main_color;
            lv_area_t bar_main_area = {
                .x1 = bar_x1, .y1 = bar_y1 + (int)(bar_height * 0.2),
                .x2 = bar_x2, .y2 = bar_y2
            };
            lv_draw_rect(&layer, &bar_dsc, &bar_main_area);
            
            bar_dsc.bg_color = light_color;
            lv_area_t bar_top_area = {
                .x1 = bar_x1, .y1 = bar_y1,
                .x2 = bar_x2, .y2 = bar_y1 + (int)(bar_height * 0.2)
            };
            lv_draw_rect(&layer, &bar_dsc, &bar_top_area);
            
            bar_dsc.bg_color = dark_color;
            lv_area_t bar_bottom_area = {
                .x1 = bar_x1, .y1 = bar_y2 - 1,
                .x2 = bar_x2, .y2 = bar_y2
            };
            lv_draw_rect(&layer, &bar_dsc, &bar_bottom_area);
            
            int peak_y = base_y - (int)app->_peak[ch][i];
            
            lv_draw_rect_dsc_t peak_dsc;
            lv_draw_rect_dsc_init(&peak_dsc);
            uint16_t peak_hue = (uint16_t)(hue_base + i * hue_step + 20);
            peak_dsc.bg_color = lv_color_hsv_to_rgb(peak_hue, 100, 100);
            peak_dsc.bg_opa = LV_OPA_90;
            
            lv_area_t peak_area = {
                .x1 = bar_x1 + 1, .y1 = peak_y - 2,
                .x2 = bar_x2 - 1, .y2 = peak_y
            };
            lv_draw_rect(&layer, &peak_dsc, &peak_area);
        }
    }
    
    lv_canvas_finish_layer(app->_canvas, &layer);
}

} // namespace esp_brookesia::apps
