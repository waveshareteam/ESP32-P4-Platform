# Factory firmware

[English](README.md) | [简体中文](README_ZH.md)

This directory provides prebuilt factory firmware and flashing instructions for ESP32-P4 development boards. [Firmware manifest](manifest.json) maps each board to its documentation and display images.

| Board | Networking | Flash | Factory config | Firmware downloads |
| --- | --- | ---: | --- | --- |
| [ESP32-P4-NANO](https://docs.waveshare.com/ESP32-P4-NANO) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` | [wifi-eth-16](wifi-eth-16/README.md) |
| [ESP32-P4-Module-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Module-DEV-KIT) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` | [wifi-eth-16](wifi-eth-16/README.md) |
| [ESP32-P4-WIFI6-DEV-KIT](https://docs.waveshare.com/ESP32-P4-WIFI6-DEV-KIT) | Wi-Fi + Ethernet | 16 MB | `wifi-eth-16.defaults` | [wifi-eth-16](wifi-eth-16/README.md) |
| [ESP32-P4-WIFI6-POE-ETH](https://docs.waveshare.com/ESP32-P4-WIFI6-POE-ETH) | Wi-Fi + Ethernet | 32 MB | `wifi-eth-32.defaults` | [wifi-eth-32](wifi-eth-32/README.md) |
| [ESP32-P4-WIFI6](https://docs.waveshare.com/ESP32-P4-WIFI6) | Wi-Fi | 32 MB | `wifi-32.defaults` | [wifi-32](wifi-32/README.md) |
| [ESP32-P4-ETH](https://docs.waveshare.com/ESP32-P4-ETH) | Ethernet | 32 MB | `eth-32.defaults` | [eth-32](eth-32/README.md) |
| [ESP32-P4-Pico](https://docs.waveshare.com/ESP32-P4-Pico) | No onboard network | 32 MB | `offline-32.defaults` | [offline-32](offline-32/README.md) |
| [ESP32-P4-Core-DEV-KIT](https://docs.waveshare.com/ESP32-P4-Core-DEV-KIT) | No onboard network | 32 MB | `offline-32.defaults` + `core-dev-kit.defaults` | [core-dev-kit](core-dev-kit/README.md) (no onboard audio or microSD) |

## Selection and flashing

- **Check hardware details**: Match the board model, chip revision, flash capacity, and display panel before downloading. These configurations target rev3.x chips; boards with the same model name may ship with different chip revisions.
- **Match the full panel model**: Select the BIN whose filename contains your panel model. `7-DSI-TOUCH-A` uses 720 × 1280; `8-DSI-TOUCH-A` and `10.1-DSI-TOUCH-A` both use 800 × 1280 but have different panel initialization settings. See the [third-party notices](../docs/THIRD_PARTY.md) for Xiaozhi and WakeNet assets.
- **Back up before flashing**: Flashing a combined BIN overwrites existing applications and settings in flash, so back up data as needed first. Flash all combined BINs at `0x0` using the files and commands in the "Firmware downloads" column above.

The `20261125` images use **115200 baud (8N1)** on the board’s USB-to-UART port for bootloader and application logs.

## Build from source

See [Factory source and build options](../examples/factory/README.md) for source details and build examples. In `examples/factory/`, combine `sdkconfig.defaults`, `sdkconfig.rev3_x.defaults`, the board defaults under `config/` from the table above, and the panel defaults below in that order:

| Panel | Resolution | Final defaults file |
| --- | --- | --- |
| 7-DSI-TOUCH-A | 720 × 1280 | `config/lcd-7-a.defaults` |
| 8-DSI-TOUCH-A | 800 × 1280 | `config/lcd-8-a.defaults` |
| 10.1-DSI-TOUCH-A | 800 × 1280 | Selected by `sdkconfig.defaults` |

1. Use a separate `-B` directory and `SDKCONFIG` path for each board and panel combination, then run `idf.py build`.
2. After `idf.py build` succeeds, run the following command to generate the merged BIN (networked builds with audio include the Xiaozhi wake model in the same BIN):

   ```bash
   idf.py -B <build-dir> merge-bin -f raw -t 0x0 -o <absolute-output-path>.bin
   ```
