# GPIO Interrupt

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Pass GPIO edge events from an interrupt handler to a task, with software debounce.

## Hardware Required

An ESP32-P4 board, a USB data cable and a button or jumper on the configured input.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Select the input GPIO and debounce time in Example Configuration.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

Pressing and releasing the input produces level and timestamp messages.

## Troubleshooting

The input uses a pull-up and detects both edges; configure the debounce interval for your switch.
