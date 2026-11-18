# Camera to LCD

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Capture MIPI-CSI frames, process them with PPA and display them through the BSP LCD path.

## Hardware Required

An ESP32-P4 board with PSRAM, a compatible DSI display, a supported MIPI-CSI camera and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Select the LCD, camera sensor and sensor output format in menuconfig.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The LCD shows live camera frames and the serial log reports the selected video format.

## Troubleshooting

Camera buffers require PSRAM; the supplied defaults select OV5647 RAW8 800 x 1280, so another sensor needs matching configuration.
