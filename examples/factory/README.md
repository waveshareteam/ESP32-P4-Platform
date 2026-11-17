# Factory Demo Source

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Run the ESP-Brookesia phone UI with calculator, drawing, spectrum, music, camera, video and settings apps.

## Hardware Required

An ESP32-P4 board with PSRAM and the display, codec, camera and storage required by the selected apps.

## How to Run

Use the factory ESP-IDF version in [product.json](../../.github/product.json), select the actual LCD in menuconfig and build in this directory. The existing defaults select rev3_x silicon, a 10.1-inch 800 x 1280 display and 32 MB flash.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The phone launcher appears and provides the installed demo apps.

## Troubleshooting

Keep `spiffs/music/` with the project: CMake packs these audio files into the storage partition. For pre-v3 silicon, use the separate legacy defaults described in the [revision guide](../../docs/ESP32P4_REVISION_CONFIG.md).
