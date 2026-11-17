# ESP-Brookesia Phone

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Run the ESP-Brookesia phone UI and registered apps on the Waveshare display BSP.

## Hardware Required

An ESP32-P4 board with PSRAM, a compatible DSI touch display and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Select the connected LCD and review the ESP-Brookesia options in menuconfig.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The backlight turns on and the phone UI appears with a periodically updated status-bar clock.

## Troubleshooting

The default panel is the 7-inch portrait model; select your actual LCD in the BSP menu and check the source anti-tearing note before changing display buffering.
