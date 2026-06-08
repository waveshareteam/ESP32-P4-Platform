# ESP32-P4 Platform Examples

[中文版本](./README_CN.md)

Open-source examples and board bring-up references for Waveshare ESP32-P4
development boards.

This repository collects ESP-IDF and Arduino examples that exercise common
ESP32-P4 platform features, including GPIO, I2C, Wi-Fi, Ethernet, SD card,
audio codec, display, LVGL, camera, video streaming, and ESP-Brookesia UI
workflows.

## Supported Boards

| Board | Product |
| --- | --- |
| ESP32-P4-NANO | <a href="https://www.waveshare.com/esp32-p4-nano.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-nano-1.jpg" width="150"></a> |
| ESP32-P4-Module-DEV-KIT | <a href="https://www.waveshare.com/esp32-p4-module-dev-kit.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-module-dev-kit-0.jpg" width="150"></a> |
| ESP32-P4-WIFI6-DEV-KIT | <a href="https://www.waveshare.com/esp32-p4-wifi6-dev-kit.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-dev-kit-1.jpg" width="150"></a> |
| ESP32-P4-WIFI6 | <a href="https://www.waveshare.com/esp32-p4-wifi6.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-1.jpg" width="150"></a> |
| ESP32-P4-ETH | <a href="https://www.waveshare.com/esp32-p4-eth.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-eth-1.jpg" width="150"></a> |
| ESP32-P4-Pico | <a href="https://www.waveshare.com/esp32-p4-pico.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-pico-1.jpg" width="150"></a> |
| ESP32-P4-WIFI6-POE-ETH | <a href="https://www.waveshare.com/esp32-p4-wifi6-poe-eth.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-poe-eth-1_2.jpg" width="150"></a> |
| ESP32-P4-Core-DEV-KIT | <a href="https://www.waveshare.com/product/arduino/boards-kits/esp32-p4/esp32-p4-core-dev-kit.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-core-dev-kit-1.jpg" width="150"></a> |

## Repository Layout

```text
.
├── examples/
│   ├── esp-idf/      ESP-IDF examples, one project per directory
│   └── arduino/      Arduino sketches and bundled example libraries
├── config/           Shared ESP-IDF configuration overlays
├── docs/             Repository structure and maintainer notes
├── .github/          Issue and pull request templates
├── CONTRIBUTING.md   Contribution workflow
├── SUPPORT.md        Support channels
├── SECURITY.md       Vulnerability reporting policy
└── LICENSE.txt       Apache-2.0 license
```

Start with [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) if this is your
first time using the repository. See
[docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md) for more details about
where new examples and documentation should live.

## Documentation

| Document | Audience | Purpose |
| --- | --- | --- |
| [Getting Started](docs/GETTING_STARTED.md) | Beginners | First build, flash, and monitor workflow |
| [Examples Guide](docs/EXAMPLES_GUIDE.md) | All users | Choose examples by difficulty and peripheral |
| [ESP32-P4 Revision Config](docs/ESP32P4_REVISION_CONFIG.md) | All users | Choose v3.x production or pre-v3 sample build profiles |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | All users | Build, flash, runtime, and peripheral checks |
| [Continuous Integration](docs/CI.md) | Contributors | GitHub Actions coverage for ESP-IDF examples |
| [Example Roadmap](docs/EXAMPLE_ROADMAP.md) | Contributors | Suggested examples that would improve coverage |
| [Project Structure](docs/PROJECT_STRUCTURE.md) | Contributors | Repository layout and example expectations |

## Quick Start

### ESP-IDF Examples

1. Install ESP-IDF. Most examples in this repository were prepared for
   ESP-IDF release/v5.4 or later. Some advanced UI examples also document
   release/v5.3 compatibility in their own README files.
2. Clone this repository.
3. Start with [00_board_check](examples/esp-idf/00_board_check/) if you are
   new to the board or this repository.
4. Set the target and build:

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
```

5. If you are building for early ESP32-P4 engineering samples earlier than
   rev v3.0, or you need to pin production firmware to rev v3.0 or v3.1,
   read [ESP32-P4 Revision Config](docs/ESP32P4_REVISION_CONFIG.md) before
   flashing.
6. Flash and monitor, replacing `PORT` with your serial port:

```bash
idf.py -p PORT flash monitor
```

Run `idf.py menuconfig` before building examples that need board-specific
settings, network credentials, display options, camera sensors, or PHY
configuration.

### Arduino Examples

The Arduino examples are under [examples/arduino](examples/arduino). Start
with [examples/arduino/README.md](examples/arduino/README.md) for the
recommended Arduino-ESP32 core, bundled LVGL version, Arduino_GFX version,
and the I2C driver compatibility note.

## Example Index

| No. | Example | Area |
| --- | --- | --- |
| 00 | [board_check](examples/esp-idf/00_board_check/) | First-run board and toolchain check |
| 01 | [HowToCreateProject](examples/esp-idf/01_HowToCreateProject/) | Minimal ESP-IDF project layout |
| 02 | [HelloWorld](examples/esp-idf/02_HelloWorld/) | Basic ESP-IDF application |
| 03 | [nvs_counter](examples/esp-idf/03_nvs_counter/) | Persistent settings with NVS |
| 04 | [freertos_tasks](examples/esp-idf/04_freertos_tasks/) | Tasks and queues |
| 05 | [gpio_io](examples/esp-idf/05_gpio_io/) | GPIO input/output |
| 06 | [gpio_interrupt](examples/esp-idf/06_gpio_interrupt/) | GPIO interrupt and debounce |
| 07 | [uart_loopback](examples/esp-idf/07_uart_loopback/) | UART send/receive loopback |
| 08 | [i2c_tools](examples/esp-idf/08_i2c_tools/) | I2C scanning and command-line tools |
| 09 | [sdmmc](examples/esp-idf/09_sdmmc/) | SD card and SDMMC |
| 10 | [wifistation](examples/esp-idf/10_wifistation/) | Wi-Fi station connection |
| 11 | [ethernetbasic](examples/esp-idf/11_ethernetbasic/) | Ethernet bring-up |
| 12 | [I2SCodec](examples/esp-idf/12_I2SCodec/) | I2S audio codec |
| 13 | [Displaycolorbar](examples/esp-idf/13_Displaycolorbar/) | LCD display color bar |
| 14 | [lvgl_demo_v9](examples/esp-idf/14_lvgl_demo_v9/) | LVGL v9 display demo |
| 15 | [eth2ap](examples/esp-idf/15_eth2ap/) | Ethernet-to-AP networking |
| 16 | [video_lcd_display](examples/esp-idf/16_video_lcd_display/) | Camera video to LCD |
| 17 | [simple_video_server](examples/esp-idf/17_simple_video_server/) | HTTP camera streaming |
| 18 | [esp_brookesia_phone](examples/esp-idf/18_esp_brookesia_phone/) | ESP-Brookesia phone UI |
| 19 | [system_monitor](examples/esp-idf/19_system_monitor/) | Serial diagnostics and runtime monitor |

For a fuller map of ESP-IDF and Arduino examples, see
[examples/README.md](examples/README.md) and
[docs/EXAMPLES_GUIDE.md](docs/EXAMPLES_GUIDE.md).

## ESP-IDF Compatibility Matrix

Legend: `✅` means the example is expected to run on that board. `❌` means
the required peripheral is not available or is not supported by the example.

| No. | Example | ESP32-P4-NANO | ESP32-P4-Module-DEV-KIT | ESP32-P4-WIFI6-DEV-KIT | ESP32-P4-WIFI6 | ESP32-P4-ETH | ESP32-P4-Pico | ESP32-P4-WIFI6-POE-ETH | ESP32-P4-Core-DEV-KIT |
| --- | --- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| - | Product page | <a href="https://www.waveshare.com/esp32-p4-nano.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-nano-1.jpg" width="120"></a> | <a href="https://www.waveshare.com/esp32-p4-module-dev-kit.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-module-dev-kit-0.jpg" width="120"></a> | <a href="https://www.waveshare.com/esp32-p4-wifi6-dev-kit.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-dev-kit-1.jpg" width="120"></a> | <a href="https://www.waveshare.com/esp32-p4-wifi6.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-1.jpg" width="120"></a> | <a href="https://www.waveshare.com/esp32-p4-eth.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-eth-1.jpg" width="120"></a> | <a href="https://www.waveshare.com/esp32-p4-pico.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-pico-1.jpg" width="120"></a> | <a href="https://www.waveshare.com/esp32-p4-wifi6-poe-eth.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-wifi6-poe-eth-1_2.jpg" width="120"></a> | <a href="https://www.waveshare.com/product/arduino/boards-kits/esp32-p4/esp32-p4-core-dev-kit.htm"><img src="https://www.waveshare.com/media/catalog/product/cache/1/image/800x800/9df78eab33525d08d6e5fb8d27136e95/e/s/esp32-p4-core-dev-kit-1.jpg" width="120"></a> |
| 00 | [board_check](examples/esp-idf/00_board_check/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 01 | [HowToCreateProject](examples/esp-idf/01_HowToCreateProject/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 02 | [HelloWorld](examples/esp-idf/02_HelloWorld/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 03 | [nvs_counter](examples/esp-idf/03_nvs_counter/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 04 | [freertos_tasks](examples/esp-idf/04_freertos_tasks/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 05 | [gpio_io](examples/esp-idf/05_gpio_io/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 06 | [gpio_interrupt](examples/esp-idf/06_gpio_interrupt/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 07 | [uart_loopback](examples/esp-idf/07_uart_loopback/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 08 | [i2c_tools](examples/esp-idf/08_i2c_tools/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 09 | [sdmmc](examples/esp-idf/09_sdmmc/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 10 | [wifistation](examples/esp-idf/10_wifistation/) | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ | ❌ |
| 11 | [ethernetbasic](examples/esp-idf/11_ethernetbasic/) | ✅ | ✅ | ✅ | ❌ | ✅ | ❌ | ✅ | ❌ |
| 12 | [I2SCodec](examples/esp-idf/12_I2SCodec/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| 13 | [Displaycolorbar](examples/esp-idf/13_Displaycolorbar/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 14 | [lvgl_demo_v9](examples/esp-idf/14_lvgl_demo_v9/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 15 | [eth2ap](examples/esp-idf/15_eth2ap/) | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ | ✅ | ❌ |
| 16 | [video_lcd_display](examples/esp-idf/16_video_lcd_display/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 17 | [simple_video_server](examples/esp-idf/17_simple_video_server/) | ✅ | ✅ | ✅ | ✅ | ✅, Ethernet | ❌ | ✅ | ❌ |
| 18 | [esp_brookesia_phone](examples/esp-idf/18_esp_brookesia_phone/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 19 | [system_monitor](examples/esp-idf/19_system_monitor/) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md)
before opening a pull request. Useful contributions include:

- Fixing build issues on a supported board.
- Adding board-specific notes to an existing example README.
- Improving setup instructions or troubleshooting notes.
- Adding focused examples for peripherals that are not covered yet.

When reporting problems, please include the board name, ESP-IDF or
Arduino-ESP32 version, serial logs, and the exact example path.

For support channel guidance, see [SUPPORT.md](SUPPORT.md).

## Third-Party Code

Some examples include third-party components or libraries for convenience,
including LVGL, Arduino_GFX, ESP-Brookesia-related components, and Espressif
managed components. See [THIRD_PARTY.md](THIRD_PARTY.md) for the high-level
inventory and check each bundled component for its own license file.

## Security

Please do not report security vulnerabilities in public issues. Follow
[SECURITY.md](SECURITY.md) for responsible disclosure.

## License

This repository is licensed under the Apache License 2.0. See
[LICENSE.txt](LICENSE.txt) for details.
