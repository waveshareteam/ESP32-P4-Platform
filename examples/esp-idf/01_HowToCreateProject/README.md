# How to Create a Project

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Provide a minimal ESP-IDF project with an empty application entry point.

## Hardware Required

An ESP32-P4 board and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory.

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## Expected Result

The project builds and boots, but its empty app_main() does not print application messages.

## Troubleshooting

Use ../00_board_check/ when you need visible runtime diagnostics.
