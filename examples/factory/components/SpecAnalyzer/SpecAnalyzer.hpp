#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "esp_dsp.h"
#include "bsp/esp-bsp.h"
#include "product_support.h"
#include "factory_ui.hpp"

namespace esp_brookesia::apps
{
    class SpecAnalyzer : public systems::phone::App
    {
    public:
        static SpecAnalyzer *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);
        ~SpecAnalyzer();

        using systems::phone::App::endRecordResource;
        using systems::phone::App::startRecordResource;

    protected:
        SpecAnalyzer(bool use_status_bar, bool use_navigation_bar);

        bool run(void) override;
        bool back(void) override;
        bool close(void) override;
        bool init(void) override;
        bool deinit(void) override;
        bool pause(void) override;
        bool resume(void) override;

    private:
        static SpecAnalyzer *_instance;

        // Audio pipeline parameters. The current hardware renders one microphone, while
        // the BSP recorder still returns interleaved stereo frames.
        static constexpr uint16_t N_SAMPLES = 1024;       // Samples per channel
        static constexpr uint16_t I2S_CHANNELS = 2;       // BSP capture frame layout: L/R interleaved
        static constexpr uint16_t MIC_COUNT = 1;          // Only one microphone is visualized
        static constexpr uint16_t STRIPE_COUNT = 48;      // Bar count for the visualizer
        static constexpr int PANEL_PAD = 24;
        static constexpr uint16_t CANVAS_WIDTH = factory_ui::WIDTH - 2 * (factory_ui::GUTTER + PANEL_PAD);
        static constexpr uint16_t CANVAS_HEIGHT = 560;

        enum class ViewMode { MIC, BARS, RADIAL, HALO, COUNT };
        static constexpr int MODE_COUNT = static_cast<int>(ViewMode::COUNT);

        lv_obj_t *_root = nullptr;
        lv_obj_t *_canvas;
        lv_obj_t *_mic_label;
        lv_obj_t *_mic_level = nullptr;
        lv_obj_t *_frequency_axis = nullptr;
        lv_obj_t *_mode_buttons[MODE_COUNT] = {};
        ViewMode _mode = ViewMode::MIC;
        unsigned _label_frames = 0;
        lv_timer_t *_timer;
        TaskHandle_t _audio_task_handle;
        bool _audio_task_running;

        // Single-microphone visualization buffers.
        __attribute__((aligned(16))) int16_t _raw_data[N_SAMPLES * I2S_CHANNELS]; // Raw BSP interleaved samples
        __attribute__((aligned(16))) float _audio_buffer[MIC_COUNT][N_SAMPLES];   // Normalized microphone samples
        __attribute__((aligned(16))) float _wind[N_SAMPLES];                      // Shared Hann window
        __attribute__((aligned(16))) float _fft_buffer[MIC_COUNT][N_SAMPLES * 2]; // Complex FFT input
        __attribute__((aligned(16))) float _spectrum[MIC_COUNT][N_SAMPLES / 2];   // Spectrum in dB
        float _display_spectrum[MIC_COUNT][STRIPE_COUNT];                         // Mapped spectrum for bars
        float _peak[MIC_COUNT][STRIPE_COUNT];                                     // Peak marker position
        float _smooth_spectrum[MIC_COUNT][STRIPE_COUNT];                          // Smoothed display values
        portMUX_TYPE _input_lock = portMUX_INITIALIZER_UNLOCKED;
        int _mic_dbfs = -90;

        uint16_t *_draw_buf; // PSRAM-preferred RGB565 canvas buffer

        bool startCapture();
        void stopCapture();
        void showMode(int index);
        void drawBars(lv_layer_t &layer, const float *levels, bool mic_test) const;
        void drawRadial(lv_layer_t &layer, const float *levels, bool halo) const;
        static void view_gesture_cb(lv_event_t *event);
        static void view_page_cb(lv_event_t *event);
        static void audio_fft_task(void *pvParameters);
        static void timer_cb(lv_timer_t *timer);
    };
}
