#include "factory_phone.hpp"
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
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>
#define APP_NAME "SpecAnalyzer"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

LV_IMG_DECLARE(img_app_specanalyzer);

namespace esp_brookesia::apps {

static constexpr const char *MODE_NAMES[] = {"Mic Test", "Bars", "Radial", "Halo"};
static constexpr uint32_t PLOT_BACKGROUND = 0x101620;

SpecAnalyzer *SpecAnalyzer::_instance = nullptr;

SpecAnalyzer *SpecAnalyzer::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new SpecAnalyzer(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

SpecAnalyzer::SpecAnalyzer(bool use_status_bar, bool use_navigation_bar) :
    App(factory_ui::screen_config(APP_NAME, &img_app_specanalyzer), factory_ui::app_config(use_status_bar, use_navigation_bar)),
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
    for (auto &channel : _smooth_spectrum) {
        std::fill_n(channel, STRIPE_COUNT, -90.0f);
    }
}

SpecAnalyzer::~SpecAnalyzer()
{
    close();
}

bool SpecAnalyzer::run(void)
{
    ESP_UTILS_LOGD("Run");

    const auto view = factory_ui::page(lv_screen_active(), "Spectrum", nullptr, nullptr);
    _root = view.root;
    lv_obj_remove_flag(view.content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(_root, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(_root, view_gesture_cb, LV_EVENT_GESTURE, this);

    const auto panel = [content = view.content](int height, int pad) {
        auto *object = factory_ui::plain(content);
        lv_obj_set_size(object, lv_pct(100), height);
        lv_obj_set_style_bg_color(object, lv_color_hex(factory_ui::CARD), 0);
        lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(object, 24, 0);
        lv_obj_set_style_pad_all(object, pad, 0);
        return object;
    };
    auto *tabs = panel(84, 8);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(tabs, 8, 0);
    for (int i = 0; i < MODE_COUNT; ++i) {
        auto *target = factory_ui::plain(tabs);
        _mode_buttons[i] = target;
        lv_obj_set_height(target, lv_pct(100));
        lv_obj_set_flex_grow(target, 1);
        lv_obj_set_style_radius(target, 16, 0);
        lv_obj_set_style_bg_color(target, lv_color_hex(0x1B3553), LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(target, lv_color_hex(factory_ui::PRESSED), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(target, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_add_flag(target, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(target, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(target, view_page_cb, LV_EVENT_CLICKED, this);
        lv_obj_center(factory_ui::label(target, MODE_NAMES[i], &lv_font_montserrat_24));
    }

    auto *meter = panel(124, PANEL_PAD);
    factory_ui::label(meter, "Microphone", &lv_font_montserrat_28);
    _mic_label = factory_ui::label(meter, "-- dBFS", &lv_font_montserrat_28, factory_ui::HINT);
    lv_obj_align(_mic_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    _mic_level = lv_bar_create(meter);
    lv_obj_remove_style_all(_mic_level);
    lv_obj_set_size(_mic_level, lv_pct(100), 12);
    lv_obj_align(_mic_level, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_bar_set_range(_mic_level, -90, 0);
    lv_bar_set_value(_mic_level, -90, LV_ANIM_OFF);
    lv_obj_set_style_radius(_mic_level, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(_mic_level, 6, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_mic_level, lv_color_hex(0x343846), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_mic_level, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_mic_level, lv_color_hex(factory_ui::ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_mic_level, LV_OPA_COVER, LV_PART_INDICATOR);

    auto *plot = panel(CANVAS_HEIGHT + 124, PANEL_PAD);
    factory_ui::label(plot, "Frequency spectrum", &lv_font_montserrat_28);

    // Prefer PSRAM for the large canvas buffer and fall back to SRAM if PSRAM is exhausted.
    size_t buf_size = CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(uint16_t);
    _draw_buf = static_cast<uint16_t *>(heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM));

    if (!_draw_buf) {
        ESP_UTILS_LOGE("PSRAM allocation failed! Using fallback SRAM");
        _draw_buf = static_cast<uint16_t *>(malloc(buf_size));
    }

    if (!_draw_buf) {
        ESP_UTILS_LOGE("Canvas buffer allocation failed!");
        close();
        return false;
    }

    // Draw into an LVGL canvas so the FFT task only updates numeric buffers and the
    // GUI timer owns all rendering work on the LVGL thread.
    _canvas = lv_canvas_create(plot);
    if (!_canvas) {
        ESP_UTILS_LOGE("Canvas creation failed!");
        close();
        return false;
    }
    lv_obj_set_size(_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_canvas_set_buffer(_canvas, _draw_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565);
    lv_canvas_fill_bg(_canvas, lv_color_hex(PLOT_BACKGROUND), LV_OPA_COVER);
    lv_obj_set_y(_canvas, 44);

    _frequency_axis = factory_ui::plain(plot);
    lv_obj_set_size(_frequency_axis, lv_pct(100), 26);
    lv_obj_align(_frequency_axis, LV_ALIGN_BOTTOM_MID, 0, 4);
    factory_ui::label(_frequency_axis, "Low frequency", &lv_font_montserrat_22, factory_ui::HINT);
    auto *high = factory_ui::label(_frequency_axis, "High frequency", &lv_font_montserrat_22, factory_ui::HINT);
    lv_obj_align(high, LV_ALIGN_TOP_RIGHT, 0, 0);
    showMode(static_cast<int>(ViewMode::MIC));

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

    if (!startCapture()) {
        close();
        return false;
    }

    return true;
}

bool SpecAnalyzer::back(void)
{
    ESP_UTILS_LOGD("Back");
    notifyCoreClosed();
    return true;
}

bool SpecAnalyzer::startCapture()
{
    for (auto &channel : _smooth_spectrum) {
        std::fill_n(channel, STRIPE_COUNT, -90.0f);
    }
    _mic_dbfs = -90;
    memset(_peak, 0, sizeof(_peak));
    _audio_task_running = true;
    if (xTaskCreatePinnedToCore(audio_fft_task, "audio_fft", 8 * 1024, this, 5,
                                &_audio_task_handle, PRO_CPU_NUM) != pdPASS) {
        ESP_UTILS_LOGE("Audio FFT task creation failed!");
        _audio_task_running = false;
        return false;
    }
    return true;
}

void SpecAnalyzer::stopCapture()
{
    _audio_task_running = false;
    for (int i = 0; i < 20 && _audio_task_handle; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (_audio_task_handle) {
        vTaskDelete(_audio_task_handle);
        _audio_task_handle = nullptr;
    }
}

bool SpecAnalyzer::close(void)
{
    ESP_UTILS_LOGD("Close");
    stopCapture();

    if (_timer) {
        lv_timer_del(_timer);
        _timer = nullptr;
    }

    if (_root) lv_obj_delete(_root);
    _root = _canvas = _mic_label = _mic_level = _frequency_axis = nullptr;
    memset(_mode_buttons, 0, sizeof(_mode_buttons));

    if (_draw_buf) {
        free(_draw_buf);
        _draw_buf = nullptr;
    }

    return true;
}

bool SpecAnalyzer::init() { return true; }
bool SpecAnalyzer::deinit() { return true; }
bool SpecAnalyzer::pause()
{
    if (_timer) lv_timer_pause(_timer);
    stopCapture();
    return true;
}

bool SpecAnalyzer::resume()
{
    if (!startCapture()) return false;
    if (_timer) lv_timer_resume(_timer);
    return true;
}

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
        float square_sum = 0;
        for (int i = 0; i < N_SAMPLES; i++) {
            size_t sample_index = has_interleaved_channels ? (i * I2S_CHANNELS) : i;
            const float sample = app->_raw_data[sample_index] / 32768.0f;
            app->_audio_buffer[0][i] = sample;
            square_sum += sample * sample;
        }
        const int mic_dbfs = static_cast<int>(lroundf(fmaxf(-90.0f, 10.0f * log10f(square_sum / N_SAMPLES + 1e-9f))));

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

            }
        }

        portENTER_CRITICAL(&app->_input_lock);
        app->_mic_dbfs = mic_dbfs;
        for (int i = 0; i < STRIPE_COUNT; ++i) {
            constexpr float smooth = 0.2f;
            app->_smooth_spectrum[0][i] = app->_smooth_spectrum[0][i] * (1.0f - smooth) +
                                          app->_display_spectrum[0][i] * smooth;
        }
        portEXIT_CRITICAL(&app->_input_lock);

        vTaskDelay(pdMS_TO_TICKS(1));
    }

    app->_audio_task_handle = nullptr;
    vTaskDelete(NULL);
}

void SpecAnalyzer::showMode(int index)
{
    index = (index + MODE_COUNT) % MODE_COUNT;
    _mode = static_cast<ViewMode>(index);
    _label_frames = 0;
    lv_obj_set_flag(_frequency_axis, LV_OBJ_FLAG_HIDDEN, _mode == ViewMode::RADIAL || _mode == ViewMode::HALO);

    for (int i = 0; i < MODE_COUNT; ++i) {
        lv_obj_set_state(_mode_buttons[i], LV_STATE_CHECKED, i == index);
        lv_obj_set_style_text_color(lv_obj_get_child(_mode_buttons[i], 0),
                                    lv_color_hex(i == index ? factory_ui::ACCENT : factory_ui::TEXT), 0);
    }
}

void SpecAnalyzer::view_gesture_cb(lv_event_t *event)
{
    auto *app = static_cast<SpecAnalyzer *>(lv_event_get_user_data(event));
    auto *touch = lv_indev_active();
    if (!touch) return;

    const lv_dir_t direction = lv_indev_get_gesture_dir(touch);
    if (direction == LV_DIR_LEFT || direction == LV_DIR_RIGHT) {
        const int offset = direction == LV_DIR_LEFT ? 1 : -1;
        app->showMode(static_cast<int>(app->_mode) + offset);
        lv_indev_wait_release(touch);
    }
}

void SpecAnalyzer::view_page_cb(lv_event_t *event)
{
    auto *app = static_cast<SpecAnalyzer *>(lv_event_get_user_data(event));
    auto *target = static_cast<lv_obj_t *>(lv_event_get_current_target(event));
    app->showMode(static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(target))));
}

static lv_color_t bandColor(int band, int count, uint8_t value = 100)
{
    return lv_color_hsv_to_rgb(190 + band * 140 / count, 100, value);
}

static void drawArea(lv_layer_t &layer, const lv_area_t &area, lv_color_t color, int radius = 0)
{
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = color;
    rect.bg_opa = LV_OPA_COVER;
    rect.radius = radius;
    lv_draw_rect(&layer, &rect, &area);
}

void SpecAnalyzer::drawBars(lv_layer_t &layer, const float *levels, bool mic_test) const
{
    constexpr int pitch = (CANVAS_WIDTH - 40) / STRIPE_COUNT;
    constexpr int left = (CANVAS_WIDTH - pitch * STRIPE_COUNT) / 2;
    const int baseline = mic_test ? CANVAS_HEIGHT - 32 : CANVAS_HEIGHT * 7 / 10;
    const int amplitude = baseline - 32;

    for (int i = 1; i <= 4; ++i) {
        const int y = baseline - amplitude * i / 4;
        drawArea(layer, {left, y, CANVAS_WIDTH - left - 1, y}, lv_color_hex(0x2A3242));
    }
    drawArea(layer, {left, baseline + 3, CANVAS_WIDTH - left - 1, baseline + 4}, lv_color_hex(0x404040));
    for (int i = 0; i < STRIPE_COUNT; ++i) {
        const int h = 4 + static_cast<int>(levels[i] * amplitude);
        const int x = left + i * pitch;
        const int right = x + pitch - 3;
        drawArea(layer, {x, baseline - h, right, baseline}, bandColor(i, STRIPE_COUNT));
        const int peak = baseline - 5 - static_cast<int>(_peak[0][i] * amplitude);
        drawArea(layer, {x, peak - 2, right, peak}, lv_color_white());
        if (!mic_test) {
            drawArea(layer, {x, baseline + 7, right, baseline + 7 + h / 3}, bandColor(i, STRIPE_COUNT, 28));
        }
    }
}

void SpecAnalyzer::drawRadial(lv_layer_t &layer, const float *levels, bool halo) const
{
    constexpr int size = std::min(CANVAS_WIDTH, CANVAS_HEIGHT);
    constexpr int cx = CANVAS_WIDTH / 2;
    constexpr int cy = CANVAS_HEIGHT / 2;
    constexpr float base = size * 0.22f;
    constexpr float amplitude = size * 0.22f;

    const auto point = [](float angle, float radius) -> lv_point_precise_t {
        return {static_cast<lv_value_precise_t>(lroundf(cx + cosf(angle) * radius)),
                static_cast<lv_value_precise_t>(lroundf(cy + sinf(angle) * radius))};
    };
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.opa = LV_OPA_COVER;
    line.width = halo ? 4 : 5;
    line.round_start = line.round_end = 1;

    if (!halo) {
        const int radius = size / 8 + static_cast<int>(levels[0] * size / 16);
        drawArea(layer, {cx - radius, cy - radius, cx + radius, cy + radius}, lv_color_hex(0x123341), LV_RADIUS_CIRCLE);
        drawArea(layer, {cx - radius + 5, cy - radius + 5, cx + radius - 5, cy + radius - 5},
                 lv_color_hex(0x091C29), LV_RADIUS_CIRCLE);
    }

    const int points = halo ? 2 * STRIPE_COUNT : STRIPE_COUNT;
    const float step = 2 * std::numbers::pi_v<float> / points;
    const auto band_at = [points, halo](int i) {
        i %= points;
        const int mirrored = std::min(i, points - i - 1);
        return halo ? mirrored : mirrored * 2;
    };
    for (int i = 0; i < points; ++i) {
        const int band = band_at(i);
        const float angle = i * step - std::numbers::pi_v<float> / 2;
        const float next = angle + step;
        line.color = bandColor(band, STRIPE_COUNT);
        line.p1 = point(angle, base + (halo ? levels[band] * amplitude : 0.0f));
        line.p2 = halo ? point(next, base + levels[band_at(i + 1)] * amplitude)
                       : point(angle, base + 8 + levels[band] * amplitude);
        lv_draw_line(&layer, &line);

        if (halo) {
            auto peak = line;
            peak.color = bandColor(band, STRIPE_COUNT, 46);
            peak.width = 2;
            peak.p1 = point(angle, base + 12 + _peak[0][band] * amplitude);
            peak.p2 = point(next, base + 12 + _peak[0][band_at(i + 1)] * amplitude);
            lv_draw_line(&layer, &peak);
        } else {
            const auto peak = point(angle, base + 12 + _peak[0][band] * amplitude);
            drawArea(layer, {peak.x - 2, peak.y - 2, peak.x + 2, peak.y + 2}, lv_color_white(), LV_RADIUS_CIRCLE);
        }
    }
}

void SpecAnalyzer::timer_cb(lv_timer_t *timer)
{
    auto *app = static_cast<SpecAnalyzer *>(lv_timer_get_user_data(timer));
    float levels[STRIPE_COUNT];
    int dbfs;
    portENTER_CRITICAL(&app->_input_lock);
    memcpy(levels, app->_smooth_spectrum[0], sizeof(levels));
    dbfs = app->_mic_dbfs;
    portEXIT_CRITICAL(&app->_input_lock);

    for (int i = 0; i < STRIPE_COUNT; ++i) {
        levels[i] = powf(std::clamp((levels[i] + 90.0f) / 90.0f, 0.0f, 1.0f), 0.8f);
        float &peak = app->_peak[0][i];
        peak = fmaxf(levels[i], peak - 0.003f - peak * 0.008f);
    }

    if (app->_label_frames++ % 6 == 0) {
        const auto color = lv_color_hex(dbfs >= -1 ? 0xFF9800 : factory_ui::ACCENT);
        lv_label_set_text_fmt(app->_mic_label, "%d dBFS", dbfs);
        lv_obj_set_style_text_color(app->_mic_label, color, 0);
        lv_obj_set_style_bg_color(app->_mic_level, color, LV_PART_INDICATOR);
    }
    lv_bar_set_value(app->_mic_level, dbfs, LV_ANIM_OFF);

    lv_canvas_fill_bg(app->_canvas, lv_color_hex(PLOT_BACKGROUND), LV_OPA_COVER);
    lv_layer_t layer;
    lv_canvas_init_layer(app->_canvas, &layer);
    switch (app->_mode) {
    case ViewMode::MIC:
        app->drawBars(layer, levels, true);
        break;
    case ViewMode::BARS:
        app->drawBars(layer, levels, false);
        break;
    case ViewMode::RADIAL:
        app->drawRadial(layer, levels, false);
        break;
    case ViewMode::HALO:
        app->drawRadial(layer, levels, true);
        break;
    case ViewMode::COUNT:
        break;
    }
    lv_canvas_finish_layer(app->_canvas, &layer);
}

} // namespace esp_brookesia::apps
