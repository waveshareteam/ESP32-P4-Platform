# Board Check

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Print chip, flash, PSRAM and heap information, then emit an alive message every five seconds.

## Hardware Required

An ESP32-P4 board and a USB data cable; no external peripheral is required.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory.

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## Expected Result

The serial monitor shows the board-check banner, silicon revision and periodic alive logs.

## Troubleshooting

The default profile targets v3.x silicon; use the explicit legacy profile described in the revision guide for pre-v3 boards. See the [revision guide](../../../docs/ESP32P4_REVISION_CONFIG.md).
