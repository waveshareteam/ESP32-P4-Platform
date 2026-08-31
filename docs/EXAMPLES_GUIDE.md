# Examples Guide

[中文版本](./EXAMPLES_GUIDE_CN.md)

The repository serves two kinds of users:

- Beginners who need a clear first success.
- Advanced users who want focused peripheral examples they can copy into real
  projects.

Use this guide to choose the right example.

Before flashing, make sure the ESP32-P4 chip revision family matches the build
profile. Production v3.x chips and earlier pre-v3 engineering samples are
mutually exclusive build targets in ESP-IDF. See
[ESP32-P4 Revision Config](ESP32P4_REVISION_CONFIG.md) for the shared
`config/` overlays and copyable commands.

## Learning Path

| Level | Goal | Example |
| --- | --- | --- |
| 0 | Verify board, flash, PSRAM, heap, and monitor | `examples/esp-idf/00_board_check` |
| 1 | Learn the standard ESP-IDF application loop | `examples/esp-idf/02_HelloWorld` |
| 2 | Learn persistent settings and tasks | `03_nvs_counter`, `04_freertos_tasks` |
| 3 | Try simple board I/O | `05_gpio_io`, `06_gpio_interrupt`, `07_uart_loopback`, `08_i2c_tools` |
| 4 | Bring up one board peripheral | Wi-Fi, Ethernet, SD card, display, or audio examples |
| 5 | Combine display, camera, network, and UI | Video and ESP-Brookesia examples |
| Arduino | Start with display output | `examples/arduino/examples/HelloWorld` |

## ESP-IDF Examples

| Example | Difficulty | External hardware | Best for |
| --- | --- | --- | --- |
| `00_board_check` | Beginner | No | First boot, flashing, serial monitor, PSRAM check |
| `01_HowToCreateProject` | Beginner | No | Understanding the smallest project structure |
| `02_HelloWorld` | Beginner | No | Classic ESP-IDF smoke test |
| `03_nvs_counter` | Beginner | No | Saving small persistent values |
| `04_freertos_tasks` | Beginner to intermediate | No | Tasks, queues, and periodic work |
| `05_gpio_io` | Beginner | Optional jumper or LED | GPIO input/output basics |
| `06_gpio_interrupt` | Beginner to intermediate | Button or jumper recommended | Interrupts and debounce |
| `07_uart_loopback` | Beginner to intermediate | Jumper wire recommended | UART send/receive workflow |
| `08_i2c_tools` | Beginner to intermediate | I2C device optional | Scanning and debugging I2C buses |
| `09_sdmmc` | Intermediate | SD card | Mounting and testing SD cards |
| `10_wifistation` | Intermediate | Wi-Fi-capable board or Wi-Fi support | Connecting to an access point |
| `11_ethernetbasic` | Intermediate | Ethernet PHY/module | Ethernet link and DHCP bring-up |
| `12_I2SCodec` | Intermediate | Audio codec | Music playback or echo mode |
| `13_Displaycolorbar` | Intermediate | Supported LCD | Display panel bring-up |
| `14_lvgl_demo_v9` | Advanced | Supported LCD, usually PSRAM | LVGL v9 UI demos |
| `15_eth2ap` | Advanced | Ethernet and Wi-Fi path | Network bridging workflow |
| `16_video_lcd_display` | Advanced | Camera and LCD | Camera to display pipeline |
| `17_simple_video_server` | Advanced | Camera and network | Browser-based camera streaming |
| `18_esp_brookesia_phone` | Advanced | LCD/touch and supported board profile | ESP-Brookesia UI application |
| `19_system_monitor` | Advanced | No | Serial diagnostics, runtime sampling, and persistent monitor settings |

## Arduino Examples

| Example | Difficulty | External hardware | Notes |
| --- | --- | --- | --- |
| `HelloWorld` | Beginner | Supported DSI display | First Arduino display test |
| `AsciiTable` | Beginner | Supported DSI display | Text rendering grid |
| `Drawing_board` | Intermediate | DSI display and touch | Touch drawing demo |
| `GFX_ESPWiFiAnalyzer` | Intermediate | DSI display and Wi-Fi | Wi-Fi scan visualization |
| `LVGLV9_Arduino` | Advanced | DSI display, touch, PSRAM | LVGL v9 demo on Arduino |
| `Camera_Preview` | Intermediate | OV5647, DSI display, PSRAM | MIPI-CSI preview using the shared Platform I2C bus |
| `Camera_ISP_Tuning` | Advanced | OV5647, DSI display, PSRAM | Non-blocking serial controls for camera ISP tuning |
| `SD_Card` | Intermediate | Inserted SD card | Four-bit SDMMC write/read check |
| `Audio_Playback` | Intermediate | ES8311 and on-board speaker | I2S tone playback; not the LCD-5 ES7210 microphone path |

## Gaps Worth Filling

The current examples cover many major peripherals, but the repository would be
friendlier with more small examples. Good next additions include:

- UART external module communication.
- ADC: single read and periodic sampling.
- SPI: simple sensor or display-independent bus scan pattern.
- Wi-Fi SoftAP: a minimal access point example.
- HTTP client: calling a public API or local server.
- Filesystem basics: SPIFFS or LittleFS without SD card hardware.
- Power management: light sleep and wake-up sources.
- Display basics split from full LVGL demos: backlight, color fill, rotation.
- Touch basics split from drawing board: raw touch coordinate logging.

When adding a new example, keep it focused on one concept and include a README
with hardware requirements, build commands, expected output, and known limits.
