# I2S Codec

[中文版本](README_ZH.md)

Audio codec example using I2S. It can run in music playback mode or echo mode,
depending on configuration.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4 board with a supported audio codec path.
- Speaker or headphones for playback mode.
- Microphone input for echo mode, if used.

## Build and Flash

```bash
cd examples/esp-idf/12_I2SCodec
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

In **Example Configuration**, check:

- Mode: music or echo.
- Microphone gain.
- Voice volume.
- BSP support option.

Default non-BSP ESP32-P4 wiring used by `main/example_config.h`:

| Signal | Default GPIO |
| --- | --- |
| I2C SDA | GPIO7 |
| I2C SCL | GPIO8 |
| I2S MCLK | GPIO13 |
| I2S BCLK | GPIO12 |
| I2S WS | GPIO10 |
| I2S DOUT | GPIO9 |
| I2S DIN | GPIO11 |
| Power amplifier enable | GPIO53 |

Enable the BSP option only when the selected BSP matches your board audio
schematic. Echo mode is available only in the non-BSP path in the current
Kconfig.

## Expected Behavior

- Music mode plays the bundled `canon.pcm` sample.
- Echo mode routes microphone input through the codec path.
- The log prints `i2s driver init success` and `es8311 codec init success`
  before audio playback or echo starts.

## Troubleshooting

- Confirm codec power, clock, I2C control bus, and I2S pins.
- Start with a moderate volume.
- If there is no sound, check whether the selected board path matches your
  hardware.
- If codec initialization fails, run [08_i2c_tools](../08_i2c_tools/) on the
  same SDA/SCL pins to confirm the ES8311 responds on the control bus.
