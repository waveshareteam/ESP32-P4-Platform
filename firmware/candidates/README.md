# Factory Firmware Candidates

[English](README.md) | [简体中文](README_ZH.md)

These 18 combined BINs passed local builds and image checks. They are available for hardware evaluation. Use the [hardware-verified manifest](../manifest.json) for released factory images.

Select a profile by board, flash capacity and panel. All candidates target ESP32-P4 chip revision v3.x and include separate builds for 7-DSI-TOUCH-A, 8-DSI-TOUCH-A and 10.1-DSI-TOUCH-A. Each profile README contains the BIN links and `0x0` flashing commands.

| Networking | Board | Candidate profile |
| --- | --- | --- |
| Wi-Fi + Ethernet | ESP32-P4-NANO, ESP32-P4-Module-DEV-KIT, ESP32-P4-WIFI6-DEV-KIT | [16 MB](wifi-eth/wifi-eth-16/README.md) |
| Wi-Fi + Ethernet | ESP32-P4-WIFI6-POE-ETH | [32 MB](wifi-eth/wifi-eth-32/README.md) |
| Wi-Fi | ESP32-P4-WIFI6 | [32 MB](wifi-only/wifi-32/README.md) |
| Ethernet | ESP32-P4-ETH | [32 MB](eth-only/eth-32/README.md) |
| Offline | ESP32-P4-Pico | [32 MB](offline/offline-32/README.md) |
| Offline | ESP32-P4-Core-DEV-KIT | [32 MB, no onboard audio or microSD](offline/core-dev-kit/README.md) |

The [candidate manifest](manifest.json) uses repository-relative paths. See the [third-party notices](../../docs/THIRD_PARTY.md) for the included Xiaozhi and WakeNet assets.
