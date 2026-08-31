/*
 * ES8311 speaker playback demo for Waveshare ESP32-P4-Platform.
 *
 * Codec I2C address: 0x18. I2S MCLK/BCLK/LRCK/DOUT: 13/12/10/9.
 * The speaker amplifier enable is GPIO53.
 */
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>
#include "esp_check.h"
#include <Waveshare_Arduino_Logging.h>
#include "es8311.h"

constexpr int PLATFORM_ES8311_ADDRESS = 0x18;
constexpr int PLATFORM_AUDIO_PA = 53;
constexpr int PLATFORM_I2S_MCLK = 13;
constexpr int PLATFORM_I2S_BCLK = 12;
constexpr int PLATFORM_I2S_LRCK = 10;
constexpr int PLATFORM_I2S_DOUT = 9;
constexpr int PLATFORM_I2C_SDA = 7;
constexpr int PLATFORM_I2C_SCL = 8;
constexpr unsigned int PLATFORM_I2C_PORT = 0;
constexpr uint32_t kSampleRate = 16000;
constexpr size_t kToneFrame = 64;
constexpr i2s_port_t kI2sPort = I2S_NUM_0;
static bool audio_ready = false;

struct Note {
  float frequency;
  uint16_t duration_ms;
};

static const Note kMelody[] = {
  {659.25f, 350}, {622.25f, 350}, {659.25f, 350}, {622.25f, 350},
  {659.25f, 350}, {493.88f, 350}, {587.33f, 350}, {523.25f, 350},
  {440.00f, 700},
};

static esp_err_t initCodec() {
  es8311_handle_t codec = es8311_create(PLATFORM_I2C_PORT, PLATFORM_ES8311_ADDRESS);
  ESP_RETURN_ON_FALSE(codec, ESP_FAIL, "ES8311", "create failed");
  const es8311_clock_config_t clock = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = kSampleRate * 256,
    .sample_frequency = kSampleRate,
  };
  ESP_RETURN_ON_ERROR(es8311_init(codec, &clock, ES8311_RESOLUTION_16,
                                  ES8311_RESOLUTION_16), "ES8311", "init failed");
  ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(codec, clock.mclk_frequency,
                                                      clock.sample_frequency),
                      "ES8311", "sample frequency failed");
  ESP_RETURN_ON_ERROR(es8311_microphone_config(codec, false), "ES8311", "disable microphone failed");
  return es8311_voice_volume_set(codec, 90, nullptr);
}

static void playNote(float frequency, uint32_t duration_ms) {
  const uint32_t total_samples = (uint64_t)kSampleRate * duration_ms / 1000;
  uint32_t played_samples = 0;
  float phase = 0;
  const float phase_step = frequency * 2.0f * PI / kSampleRate;
  int16_t frame[kToneFrame * 2];
  while (played_samples < total_samples) {
    const uint32_t chunk = min<uint32_t>(kToneFrame, total_samples - played_samples);
    for (uint32_t index = 0; index < chunk; ++index) {
      const int16_t sample = static_cast<int16_t>(sinf(phase) * 8000.0f);
      frame[index * 2] = sample;
      frame[index * 2 + 1] = sample;
      phase += phase_step;
      if (phase > 2.0f * PI) phase -= 2.0f * PI;
    }
    size_t written = 0;
    i2s_write(kI2sPort, frame, chunk * sizeof(int16_t) * 2, &written, portMAX_DELAY);
    played_samples += chunk;
  }
}

void setup() {
  waveshare::logging::beginSerialLog();
  pinMode(PLATFORM_AUDIO_PA, OUTPUT);
  digitalWrite(PLATFORM_AUDIO_PA, HIGH);
  Wire.begin(PLATFORM_I2C_SDA, PLATFORM_I2C_SCL, 100000);

  const i2s_config_t i2s_config = {
    .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = kSampleRate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
    .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,
  };
  const i2s_pin_config_t pins = {
    .mck_io_num = PLATFORM_I2S_MCLK,
    .bck_io_num = PLATFORM_I2S_BCLK,
    .ws_io_num = PLATFORM_I2S_LRCK,
    .data_out_num = PLATFORM_I2S_DOUT,
    .data_in_num = -1,
  };
  if (i2s_driver_install(kI2sPort, &i2s_config, 0, nullptr) != ESP_OK ||
      i2s_set_pin(kI2sPort, &pins) != ESP_OK || initCodec() != ESP_OK) {
    Serial.println("ES8311 playback initialization failed");
    i2s_driver_uninstall(kI2sPort);
    digitalWrite(PLATFORM_AUDIO_PA, LOW);
    return;
  }
  i2s_zero_dma_buffer(kI2sPort);
  audio_ready = true;
  Serial.println("ES8311 playback ready");
}

void loop() {
  if (!audio_ready) {
    delay(1000);
    return;
  }
  for (const Note &note : kMelody) {
    playNote(note.frequency, note.duration_ms);
    delay(60);
  }
  delay(1200);
}
