# Hello World

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Run the ESP-IDF hello-world application and print basic chip and memory information.

## Hardware Required

An ESP32-P4 board and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory.

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## Expected Result

The monitor prints Hello world!, memory statistics and a restart countdown.

## Troubleshooting

The restart after the countdown is intentional.
