# UART Loopback

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Transmit text on the configured UART and read it back on RX.

## Hardware Required

An ESP32-P4 board, a USB data cable and a jumper between the configured TX and RX GPIOs.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Check the UART port, baud rate and TX/RX GPIOs in Example Configuration.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The monitor prints the received text when TX and RX are connected.

## Troubleshooting

The loopback UART is separate from the log console; a missing jumper produces a receive-timeout warning.
