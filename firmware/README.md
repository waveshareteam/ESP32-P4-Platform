# Factory firmware

[English](README.md) | [简体中文](README_ZH.md)

[manifest.json](manifest.json) lists images verified on hardware. Match the board, chip revision, flash capacity, and panel before flashing.

| Board | Networking | Flash | Factory config |
| --- | --- | ---: | --- |
| [ESP32-P4-NANO](https://docs.waveshare.com/ESP32-P4-NANO) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` |
| [ESP32-P4-Module-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` |
| [ESP32-P4-WIFI6-DEV-KIT](https://docs.waveshare.com/ESP32-P4-WIFI6-DEV-KIT) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` |
| [ESP32-P4-WIFI6-POE-ETH](https://docs.waveshare.com/ESP32-P4-WIFI6-POE-ETH) | Wi-Fi + Ethernet | 32 MB | `wifi-eth-32.defaults` |
| [ESP32-P4-WIFI6](https://docs.waveshare.com/ESP32-P4-WIFI6) | Wi-Fi | 32 MB | `wifi-32.defaults` |
| [ESP32-P4-ETH](https://docs.waveshare.com/ESP32-P4-ETH) | Ethernet | 32 MB | `eth-32.defaults` |
| [ESP32-P4-Pico](https://docs.waveshare.com/ESP32-P4-Pico) | No onboard network | 32 MB | `offline-32.defaults` |
| [ESP32-P4-Core-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Core-DEV-KIT) | No onboard network | 32 MB | `offline-32.defaults` + `core-dev-kit.defaults` |

Build a separate image for each panel. The 7-DSI-TOUCH-A uses 720×1280. The 8-DSI-TOUCH-A and 10.1-DSI-TOUCH-A both use 800×1280 and have different panel initialization settings. These configurations target rev3.x chips. Boards with the same model name may ship with different chip revisions.

Flash a combined BIN at **`0x0`**. Back up settings or application data before writing the image. The board-specific README lists the image and flash command. [Factory source and build options](../examples/factory/README.md)

## Build from source

In `examples/factory/`, combine `sdkconfig.defaults`, `sdkconfig.rev3_x.defaults` and the board defaults above in that order. Append the panel defaults listed below.

| Panel | Resolution | Final defaults file |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720×1280 | `config/lcd-7-a.defaults` |
| 8-DSI-TOUCH-A | 800×1280 | `config/lcd-8-a.defaults` |
| 10.1-DSI-TOUCH-A | 800×1280 | Selected by `sdkconfig.defaults` |

Use a separate `-B` directory and `SDKCONFIG` path per combination. After `idf.py build` succeeds, run `idf.py -B <build-dir> merge-bin -f raw -t 0x0 -o <absolute-output-path>.bin` against that build. The combined network image includes the Xiaozhi wake model when the board has audio.

The four download groups are `wifi-eth`, `wifi-only`, `eth-only`, and `offline`. `manifest.json` uses the [multi-product manifest format](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-X/blob/main/firmware/manifest.json) with repository-relative paths.
