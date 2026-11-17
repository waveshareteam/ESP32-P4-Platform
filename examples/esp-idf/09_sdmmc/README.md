# SDMMC

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Mount an SD card, write and rename test files, read them back and unmount the card.

## Hardware Required

An ESP32-P4 board with a connected SDMMC slot, an SD card and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Check SD/MMC Example Configuration for bus width, GPIOs, formatting and SD power settings.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The monitor prints card information and file-operation results for hello.txt, foo.txt and nihao.txt.

## Troubleshooting

Formatting options erase card data; verify bus width and the board-specific SD power configuration before enabling them.
