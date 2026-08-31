# Examples

[中文版本](README_CN.md)

This directory contains both ESP-IDF projects and Arduino sketches for the
Waveshare ESP32-P4 platform boards.

## ESP-IDF

Each directory under [esp-idf](esp-idf/) is a standalone ESP-IDF project. Run
ESP-IDF commands from inside the selected example directory unless the example
README says otherwise.

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

ESP32-P4 production v3.x chips and earlier pre-v3 chips require different
ESP-IDF build profiles. The default is `rev3_x`/`MIN_300`; select legacy
`rev1_3`/`MIN_100` explicitly for pre-v3 silicon including v1.3. The examples
keep revision selection in the shared overlays under [../config](../config/).
Generated `sdkconfig` files and binaries are not interchangeable. See
[../docs/ESP32P4_REVISION_CONFIG.md](../docs/ESP32P4_REVISION_CONFIG.md) before
flashing across different chip revision families.

Managed dependency policy and the classification of example-local components
are documented in [../docs/COMPONENTS.md](../docs/COMPONENTS.md). For P4 boards
using a C6 Wi-Fi coprocessor, also read
[../docs/P4_C6_HOSTED_WIFI.md](../docs/P4_C6_HOSTED_WIFI.md).

| Directory | Purpose | Notes |
| --- | --- | --- |
| [00_board_check](esp-idf/00_board_check/) | First-run board and toolchain check | No external hardware required |
| [01_HowToCreateProject](esp-idf/01_HowToCreateProject/) | Minimal project template | Good starting point for new examples |
| [02_HelloWorld](esp-idf/02_HelloWorld/) | Basic app and logging | Quick environment check |
| [03_nvs_counter](esp-idf/03_nvs_counter/) | Persistent settings with NVS | No external hardware required |
| [04_freertos_tasks](esp-idf/04_freertos_tasks/) | FreeRTOS tasks and queues | No external hardware required |
| [05_gpio_io](esp-idf/05_gpio_io/) | GPIO input/output | Optional jumper wire or LED |
| [06_gpio_interrupt](esp-idf/06_gpio_interrupt/) | GPIO interrupt and debounce | Button or jumper recommended |
| [07_uart_loopback](esp-idf/07_uart_loopback/) | UART loopback | Jumper wire recommended |
| [08_i2c_tools](esp-idf/08_i2c_tools/) | I2C tools | Includes command handlers |
| [09_sdmmc](esp-idf/09_sdmmc/) | SD card | Requires SD card wiring/slot support |
| [10_wifistation](esp-idf/10_wifistation/) | Wi-Fi station | Requires Wi-Fi-capable board or configured Wi-Fi support |
| [11_ethernetbasic](esp-idf/11_ethernetbasic/) | Ethernet | Check PHY settings in menuconfig |
| [12_I2SCodec](esp-idf/12_I2SCodec/) | I2S audio codec | Includes audio sample data |
| [13_Displaycolorbar](esp-idf/13_Displaycolorbar/) | Display color bar | Useful for LCD bring-up |
| [14_lvgl_demo_v9](esp-idf/14_lvgl_demo_v9/) | LVGL v9 | Uses managed LVGL/BSP components |
| [15_eth2ap](esp-idf/15_eth2ap/) | Ethernet to AP | Requires both network paths |
| [16_video_lcd_display](esp-idf/16_video_lcd_display/) | Camera video to display | See the example README first |
| [17_simple_video_server](esp-idf/17_simple_video_server/) | HTTP video server | Includes frontend assets |
| [18_esp_brookesia_phone](esp-idf/18_esp_brookesia_phone/) | ESP-Brookesia UI | Includes English and Chinese README files |
| [19_system_monitor](esp-idf/19_system_monitor/) | Serial diagnostics and runtime monitor | No external hardware required |

## Arduino

Arduino examples are under [arduino/examples](arduino/examples/). The
[arduino/README.md](arduino/README.md) file documents the recommended
Arduino-ESP32 core and bundled libraries. A
[Chinese Arduino guide](arduino/README_CN.md) is also available.

| Directory | Purpose |
| --- | --- |
| [AsciiTable](arduino/examples/AsciiTable/) | Serial output example |
| [Drawing_board](arduino/examples/Drawing_board/) | Display/touch drawing example |
| [GFX_ESPWiFiAnalyzer](arduino/examples/GFX_ESPWiFiAnalyzer/) | Wi-Fi analyzer UI |
| [HelloWorld](arduino/examples/HelloWorld/) | Minimal Arduino sketch |
| [LVGLV9_Arduino](arduino/examples/LVGLV9_Arduino/) | LVGL v9 Arduino example |
| [Camera_Preview](arduino/examples/Camera_Preview/) | OV5647 camera preview; requires PSRAM and the Platform I2C bus |
| [Camera_ISP_Tuning](arduino/examples/Camera_ISP_Tuning/) | OV5647 ISP tuning; requires PSRAM and the Platform I2C bus |
| [SD_Card](arduino/examples/SD_Card/) | SD-card access; requires an inserted card |
| [Audio_Playback](arduino/examples/Audio_Playback/) | ES8311 playback using the on-board speaker |

The Arduino default is `ChipVariant=postv3` (v3.00+, 400 MHz). Use the
explicit `prev3`/360 MHz option only for legacy pre-v3 boards. Camera sketches
require a supported OV5647 and PSRAM; they reuse the Platform I2C bus. The
Platform audio surface is ES8311 output, so LCD-5's ES7210 `Mic_Record` is not
part of this product example set. Compilation is not a hardware check.

The [arduino/libraries](arduino/libraries/) directory contains bundled
libraries used by these sketches. Check each library's own metadata and
license before redistributing it separately.

## Adding a New Example

Keep new examples small and focused:

- Use a numbered ESP-IDF directory for a new standalone ESP-IDF example.
- Include `README.md`, `CMakeLists.txt`, `main/`, and `sdkconfig.defaults`
  when applicable.
- Document required hardware, menuconfig options, and expected serial output.
- Keep generated build artifacts, `sdkconfig`, `managed_components`, and
  dependency lock files out of git unless they are intentionally curated.

See [../docs/EXAMPLES_GUIDE.md](../docs/EXAMPLES_GUIDE.md) for the learning
path and [../docs/EXAMPLE_ROADMAP.md](../docs/EXAMPLE_ROADMAP.md) for suggested
future examples.
