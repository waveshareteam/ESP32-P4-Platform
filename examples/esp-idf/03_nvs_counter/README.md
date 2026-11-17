# NVS Counter

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Store a persistent boot counter in NVS.

## Hardware Required

An ESP32-P4 board and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory.

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## Expected Result

Saved boot count increases after each reset and is printed in the serial monitor.

## Troubleshooting

Erasing flash with idf.py erase-flash clears the saved counter and other flash data.
