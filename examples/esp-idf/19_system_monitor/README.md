# System Monitor

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Combine periodic memory sampling, a serial console and persistent sampling settings.

## Hardware Required

An ESP32-P4 board and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory.

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## Expected Result

The monitor> console accepts help, info, heap and period <ms>, while periodic samples report uptime and memory.

## Troubleshooting

The sampling period is saved in NVS and survives a reset.
