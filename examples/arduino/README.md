# ESP32-P4 Arduino Examples

[English](README.md) | [简体中文](README_ZH.md)

Use the bundled libraries for these sketches. [product.json](../../.github/product.json) defines the framework version and complete FQBN.

## Setup

1. Install Arduino IDE and Arduino-ESP32 `3.3.11`.
2. Set the sketchbook location to `examples/arduino/`, or copy `libraries/` into the `libraries` directory of your existing sketchbook.
3. Select `ESP32P4 Dev Module`, check the options below and upload a sketch.

| Tools menu | Setting |
| --- | --- |
| PSRAM | Enabled |
| USB Mode | USB-OTG (TinyUSB), `USBMode=default` |
| Chip Variant | post-v3 / v3.00+, `ChipVariant=postv3` |
| Flash Size | 16 MB |
| Partition Scheme | 3MB APP/9.9MB FATFS, 16 MB |

For pre-v3 silicon, explicitly select `ChipVariant=prev3` and keep its output separate; see the [revision guide](../../docs/ESP32P4_REVISION_CONFIG.md). Display sketches use `CURRENT_SCREEN` to select the 10.1-, 8- or 7-inch DSI panel; 10.1-inch is the default.

## Bundled Libraries

- `board_config/`: board display, I2C and GT911 touch integration.
- `GFX_Library_for_Arduino/`: Arduino_GFX `1.6.0`.
- `lvgl/` and `lv_conf.h`: LVGL `9.3.0` and its configuration.
- `Waveshare_Arduino_Logging/`: shared serial-log initialization.

Keep these libraries and their licenses, and avoid shadowing them with other installed copies; see [third-party attribution](../../docs/THIRD_PARTY.md). GT911 uses polling after probing `0x5D` and `0x14`, without assigning INT/RST GPIOs; sketches continue when touch is unavailable.

## Serial Startup

USB CDC On Boot is disabled in this profile and logs use UART0 at `115200`; use the programming connector identified in your board's hardware guide. Sketch startup does not require an open serial monitor.

Power-cycle with the monitor closed, confirm normal operation, then open and close the monitor while checking that the application keeps running. Wi-Fi examples require C6 slave firmware compatible with the host components; see the [P4 + C6 guide](../../docs/P4_C6_HOSTED_WIFI.md).

## Examples

The [repository homepage](../../README.md) and each example README list the sketches and hardware requirements. Camera examples use OV5647 and the shared display I2C bus; SD_Card needs a FAT32 card, and Audio_Playback uses ES8311 and a speaker.

## Firmware Packages

The shared [Build workflow](../../.github/workflows/build.yml) produces example firmware with flashing helpers and a README. See the [template firmware documentation](https://github.com/waveshareteam/esp32-repo-template/blob/main/docs/release.md) for the package format.
