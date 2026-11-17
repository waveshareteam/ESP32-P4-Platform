#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "esp_dsp.h"
#include "bsp/esp-bsp.h"
#include "product_support.h"

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
        static constexpr uint16_t CANVAS_WIDTH = BSP_LCD_H_RES;
        static constexpr uint16_t CANVAS_HEIGHT = (BSP_LCD_V_RES >= 720) ? 360 : (BSP_LCD_V_RES / 2);

        lv_obj_t *_canvas;
        lv_obj_t *_mic_label;
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

        lv_color_t *_draw_buf; // PSRAM-preferred canvas draw buffer

        static void audio_fft_task(void *pvParameters);
        static void timer_cb(lv_timer_t *timer);
    };
}
