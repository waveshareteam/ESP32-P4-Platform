# Display Color Bar

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initialize the display through the Waveshare BSP and enable the hardware color-bar generator.

## Hardware Required

An ESP32-P4 board, a compatible DSI display and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Select the connected LCD under Board Support Package(ESP32-P4).

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The backlight turns on and the LCD displays vertical color bars.

## Troubleshooting

Select the actual panel in BSP configuration before using the color-bar test.
