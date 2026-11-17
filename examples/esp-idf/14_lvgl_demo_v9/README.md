# LVGL v9 Demo

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Run the LVGL widgets demo using the Waveshare display BSP and local LVGL support component.

## Hardware Required

An ESP32-P4 board with PSRAM, a compatible DSI display and optional touch input.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Select the connected panel in the BSP menu and keep the LVGL v9 configuration supplied by this project.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The display shows the LVGL widgets UI and performance monitor.

## Troubleshooting

The application uses triple partial anti-tearing mode; check the existing source note before changing this mode on ESP-IDF 5.5.
