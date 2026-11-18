# Factory Demo Source

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Run the reference Brookesia icon launcher on a rectangular portrait display. Calculator, Draw, Spectrum, Music, Video and Settings use layouts adapted from the Knob factory UI.

## Hardware Required

An ESP32-P4 board with PSRAM and the display, codec, camera and storage required by the selected apps.

## How to Run

Use the factory ESP-IDF version in [product.json](../../.github/product.json) and select the attached LCD and the board's flash capacity in menuconfig. The defaults are a rev3_x chip, the 800 x 1280 10.1-DSI-TOUCH-A and 32 MB flash. For the 720 x 1280 7-DSI-TOUCH-A, select that panel in the BSP configuration.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

Swipe up from the bottom edge to return to the launcher. In Brookesia views, hold after swiping up to open recent apps; use a page title with a back arrow to return from a settings or playlist subpage.

| Launcher preview | Brightness control preview |
| --- | --- |
| ![800 x 1280 factory home](../../docs/images/factory-home-800x1280.png) | ![720 x 1280 brightness control](../../docs/images/factory-display-720x1280.png) |

## Troubleshooting

Keep `spiffs/music/` with the project: CMake packs these audio files into the storage partition. Wi-Fi needs C6 slave firmware compatible with the host components; see the [P4 + C6 guide](../../docs/P4_C6_HOSTED_WIFI.md).
