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
        static constexpr int PLOT_WIDTH = factory_ui::WIDTH - 2 * (factory_ui::GUTTER + PANEL_PAD);
        static constexpr int PLOT_HEIGHT = 560;
        static constexpr int BAR_PITCH = (PLOT_WIDTH - 40) / STRIPE_COUNT;
        static constexpr int BAR_LEFT = (PLOT_WIDTH - BAR_PITCH * STRIPE_COUNT) / 2;
        static constexpr int RADIAL_AMPLITUDE = PLOT_HEIGHT * 22 / 100;
        static constexpr int MIC_FLOOR = -90;

        enum class ViewMode { MIC, BARS, RADIAL, HALO, COUNT };
        static constexpr int MODE_COUNT = static_cast<int>(ViewMode::COUNT);

        struct BandPixels {
            int16_t level;
            int16_t peak;
            bool operator==(const BandPixels &) const = default;
        };

        lv_obj_t *_root = nullptr;
        lv_obj_t *_plot;
        lv_obj_t *_mic_label;
        lv_obj_t *_mic_level = nullptr;
        lv_obj_t *_frequency_axis = nullptr;
        lv_obj_t *_mode_buttons[MODE_COUNT] = {};
        ViewMode _mode = ViewMode::COUNT;
        ViewMode _drawn_mode = ViewMode::COUNT;
        BandPixels _bands[STRIPE_COUNT] = {};
        int _label_dbfs = MIC_FLOOR;
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
        int _mic_dbfs = MIC_FLOOR;

        bool startCapture();
        void stopCapture();
        void showMode(int index);
        void invalidateBars(const BandPixels *bands, bool mic_test);
        void invalidateRadial(const BandPixels *bands, bool halo);
        void drawBars(lv_layer_t &layer, const lv_area_t &area, bool mic_test) const;
        void drawRadial(lv_layer_t &layer, const lv_area_t &area, bool halo) const;
        static int barBaseline(bool mic_test);
        static void plot_draw_cb(lv_event_t *event);
        static void view_gesture_cb(lv_event_t *event);
        static void view_page_cb(lv_event_t *event);
        static void audio_fft_task(void *pvParameters);
        static void timer_cb(lv_timer_t *timer);
    };
}
