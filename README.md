<div align="center">
  <h1>Waveshare ESP32-P4 Platform</h1>
  <p><strong>Examples and bring-up resources for Waveshare ESP32-P4 development boards</strong></p>
  <p>
    <a href="https://www.waveshare.com/esp32-p4-nano.htm"><img src="assets/products/esp32-p4-nano.jpg" alt="ESP32-P4-NANO" width="94"></a>
    <a href="https://www.waveshare.com/esp32-p4-module-dev-kit.htm"><img src="assets/products/esp32-p4-module-dev-kit.jpg" alt="ESP32-P4-Module-DEV-KIT" width="94"></a>
    <a href="https://www.waveshare.com/esp32-p4-wifi6-dev-kit.htm"><img src="assets/products/esp32-p4-wifi6-dev-kit.jpg" alt="ESP32-P4-WIFI6-DEV-KIT" width="94"></a>
    <a href="https://www.waveshare.com/esp32-p4-wifi6.htm"><img src="assets/products/esp32-p4-wifi6.jpg" alt="ESP32-P4-WIFI6" width="94"></a>
    <a href="https://www.waveshare.com/esp32-p4-eth.htm"><img src="assets/products/esp32-p4-eth.jpg" alt="ESP32-P4-ETH" width="94"></a>
    <a href="https://www.waveshare.com/esp32-p4-pico.htm"><img src="assets/products/esp32-p4-pico.jpg" alt="ESP32-P4-Pico" width="94"></a>
    <a href="https://www.waveshare.com/esp32-p4-wifi6-poe-eth.htm"><img src="assets/products/esp32-p4-wifi6-poe-eth.jpg" alt="ESP32-P4-WIFI6-POE-ETH" width="94"></a>
    <a href="https://www.waveshare.com/esp32-p4-core-dev-kit.htm"><img src="assets/products/esp32-p4-core-dev-kit.jpg" alt="ESP32-P4-Core-DEV-KIT" width="94"></a>
  </p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/esp-idf-examples.yml"><img alt="ESP-IDF examples" src="https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/esp-idf-examples.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/arduino-examples.yml"><img alt="Arduino examples" src="https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/arduino-examples.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/repository-policy.yml"><img alt="Repository policy" src="https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/repository-policy.yml/badge.svg"></a>
    <a href="LICENSE.txt"><img alt="Apache-2.0 license" src="https://img.shields.io/github/license/waveshareteam/ESP32-P4-Platform"></a>
  </p>
  <p>
    <a href="README_CN.md">中文</a> ·
    <a href="https://docs.waveshare.com/ESP32-P4">🌐 ESP32-P4 Products</a> ·
    <a href="docs/GETTING_STARTED.md">📚 Product Documentation</a> ·
    <a href="examples/esp-idf/">🧩 ESP-IDF Examples</a> ·
    <a href="examples/arduino/">🔧 Arduino Examples</a>
  </p>
</div>

---

## ✨ Overview

This repository collects first-party ESP-IDF and Arduino examples for the
Waveshare ESP32-P4 development board family. The examples cover GPIO, I2C,
Wi-Fi, Ethernet, SD card, audio codec, display, LVGL, camera, video streaming,
and ESP-Brookesia UI workflows.

Because this is a multi-board platform repository, select your board below and
use its official product documentation for hardware-specific setup, interfaces,
and accessories.

The [`firmware/`](firmware/) tree contains the ESP-Brookesia source project and
prebuilt factory firmware packages. See [Firmware](firmware/README.md) for the
boundary between them.

## 🤝 Supported Boards

Select a product image to open its store page, or use the documentation link
below each board for setup and hardware details.

<table>
  <tr>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-nano.htm"><img src="assets/products/esp32-p4-nano.jpg" alt="ESP32-P4-NANO" width="170"><br><strong>ESP32-P4-NANO</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-NANO">Documentation</a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-module-dev-kit.htm"><img src="assets/products/esp32-p4-module-dev-kit.jpg" alt="ESP32-P4-Module-DEV-KIT" width="170"><br><strong>ESP32-P4-Module-DEV-KIT</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT">Documentation</a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-wifi6-dev-kit.htm"><img src="assets/products/esp32-p4-wifi6-dev-kit.jpg" alt="ESP32-P4-WIFI6-DEV-KIT" width="170"><br><strong>ESP32-P4-WIFI6-DEV-KIT</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-WIFI6-DEV-KIT">Documentation</a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-wifi6.htm"><img src="assets/products/esp32-p4-wifi6.jpg" alt="ESP32-P4-WIFI6" width="170"><br><strong>ESP32-P4-WIFI6</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-WIFI6">Documentation</a>
    </td>
  </tr>
  <tr>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-eth.htm"><img src="assets/products/esp32-p4-eth.jpg" alt="ESP32-P4-ETH" width="170"><br><strong>ESP32-P4-ETH</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-ETH">Documentation</a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-pico.htm"><img src="assets/products/esp32-p4-pico.jpg" alt="ESP32-P4-Pico" width="170"><br><strong>ESP32-P4-Pico</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-Pico">Documentation</a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-wifi6-poe-eth.htm"><img src="assets/products/esp32-p4-wifi6-poe-eth.jpg" alt="ESP32-P4-WIFI6-POE-ETH" width="170"><br><strong>ESP32-P4-WIFI6-POE-ETH</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-WIFI6-POE-ETH">Documentation</a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.waveshare.com/esp32-p4-core-dev-kit.htm"><img src="assets/products/esp32-p4-core-dev-kit.jpg" alt="ESP32-P4-Core-DEV-KIT" width="170"><br><strong>ESP32-P4-Core-DEV-KIT</strong></a><br>
      <a href="https://docs.waveshare.com/ESP32-P4-Core-DEV-KIT">Documentation</a>
    </td>
  </tr>
</table>

## 🗂️ Repository Layout

| Path | Purpose |
| --- | --- |
| [`examples/esp-idf/`](examples/esp-idf/) | First-party ESP-IDF projects |
| [`examples/arduino/`](examples/arduino/) | First-party Arduino sketches and bundled libraries |
| [`firmware/`](firmware/) | ESP-Brookesia source firmware and prebuilt factory firmware packages |
| [`assets/`](assets/) | Product images used by the documentation |
| [`config/`](config/) | Shared ESP-IDF configuration overlays |
| [`docs/`](docs/) | Getting-started, example, CI, and maintainer documentation |
| [`.github/`](.github/) | CI workflows and contribution templates |

Start with [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) if this is your
first time using the repository. See
[docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md) for more details about
where new examples and documentation should live.

## 📚 Documentation

| Document | Audience | Purpose |
| --- | --- | --- |
| [ESP32-P4 Product Documentation](https://docs.waveshare.com/ESP32-P4) | All users | Official board setup and hardware documentation |
| [Getting Started](docs/GETTING_STARTED.md) | Beginners | First build, flash, and monitor workflow |
| [Examples Guide](docs/EXAMPLES_GUIDE.md) | All users | Choose examples by difficulty and peripheral |
| [Arduino Guide](examples/arduino/README.md) | Arduino users | Board settings, bundled libraries, and example index |
| [Firmware](firmware/README.md) | All users | Source firmware and prebuilt factory package boundaries |
| [ESP32-P4 Revision Config](docs/ESP32P4_REVISION_CONFIG.md) | All users | Use default rev3_x/v3.00+ or explicit legacy rev1_3/pre-v3 profiles |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | All users | Build, flash, runtime, and peripheral checks |
| [Continuous Integration](docs/CI.md) | Contributors | GitHub Actions coverage for ESP-IDF and Arduino examples |
| [Managed Components](docs/COMPONENTS.md) | Contributors | Dependency policy and example-local component classification |
| [P4 + C6 Hosted Wi-Fi](docs/P4_C6_HOSTED_WIFI.md) | Network users | Version lanes, firmware boundary, and runtime validation |
| [Example Roadmap](docs/EXAMPLE_ROADMAP.md) | Contributors | Suggested examples that would improve coverage |
| [Project Structure](docs/PROJECT_STRUCTURE.md) | Contributors | Repository layout and example expectations |

## 🚀 Quick Start

### ESP-IDF Examples

1. Install an ESP-IDF version listed in the
   [CI guide](docs/CI.md). Check the selected example's README for any
   additional version requirements.
2. Clone this repository.
3. Start with [00_board_check](examples/esp-idf/00_board_check/) if you are
   new to the board or this repository.
4. Set the target and build:

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
```

5. The default profile is `rev3_x`/`MIN_300` for v3.x silicon. For pre-v3
   silicon, including v1.3, explicitly select legacy `rev1_3`/`MIN_100` with
   `SELECTS_REV_LESS_V3`; do not reuse generated `sdkconfig` or binaries
   between profiles. Read
   [ESP32-P4 Revision Config](docs/ESP32P4_REVISION_CONFIG.md) before
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

## 🛠️ Toolchains and CI

| Surface | Repository coverage | CI status |
| --- | --- | --- |
| ESP-IDF | First-party projects under [`examples/esp-idf/`](examples/esp-idf/) | Built for ESP-IDF `v5.5.5` and `v6.0.2` on target `esp32p4` |
| Arduino | First-party sketches and bundled libraries under [`examples/arduino/`](examples/arduino/) | Compiled with Arduino-ESP32 `3.3.11`, PSRAM, and default `ChipVariant=postv3` (v3.00+, 400 MHz); legacy pre-v3/v1.3 uses explicit `prev3`/360 MHz; successful builds produce split-file diagnostic artifacts, but still require ESP32-P4 hardware validation |

The [ESP-IDF examples workflow](https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/esp-idf-examples.yml)
discovers changed projects automatically and also supports manual runs for one
example or the full matrix. The
[Arduino examples workflow](https://github.com/waveshareteam/ESP32-P4-Platform/actions/workflows/arduino-examples.yml)
does the same for the nine first-party sketches while excluding examples
bundled inside libraries. See [Continuous Integration](docs/CI.md) for details.

## 🧪 Example Index

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

## ✅ ESP-IDF Compatibility Matrix

Legend: `✅` means the example is expected to run on that board. `❌` means
the required peripheral is not available or is not supported by the example.

> [!NOTE]
> This matrix summarizes the repository's current example support declarations.
> CI checks compilation; use the official product documentation above to verify
> board-specific interfaces and hardware setup.

| No. | Example | ESP32-P4-NANO | ESP32-P4-Module-DEV-KIT | ESP32-P4-WIFI6-DEV-KIT | ESP32-P4-WIFI6 | ESP32-P4-ETH | ESP32-P4-Pico | ESP32-P4-WIFI6-POE-ETH | ESP32-P4-Core-DEV-KIT |
| --- | --- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
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

## 🤝 Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md)
before opening a pull request. Useful contributions include:

- Fixing build issues on a supported board.
- Adding board-specific notes to an existing example README.
- Improving setup instructions or troubleshooting notes.
- Adding focused examples for peripherals that are not covered yet.

When reporting problems, please include the board name, ESP-IDF or
Arduino-ESP32 version, sanitized serial logs, and the exact example path.
Remove credentials, tokens, private network details, unique device identifiers,
and local filesystem paths before posting logs publicly.

For support channel guidance, see [SUPPORT.md](SUPPORT.md).

## 🔒 Security

Do not publish vulnerability details in public issues. Use the actionable
private reporting channel in [SECURITY.md](SECURITY.md).

## 📦 Third-Party Code

Some examples include third-party components or libraries for convenience,
including LVGL, Arduino_GFX, ESP-Brookesia-related components, and Espressif
managed components. See [THIRD_PARTY.md](THIRD_PARTY.md) for the high-level
inventory and check each bundled component for its own license file.

## 📄 License

This repository is licensed under the Apache License 2.0. See
[LICENSE.txt](LICENSE.txt) for details.
