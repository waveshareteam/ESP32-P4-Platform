# I2C Tools

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Provide a serial console for I2C scanning and register reads and writes.

## Hardware Required

An ESP32-P4 board, a USB data cable and an optional I2C device with suitable bus pull-ups.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Configure SDA, SCL and optional command-history storage in Example Configuration.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The i2c-tools> prompt accepts help, i2cdetect, i2cconfig, i2cget and i2cset.

## Troubleshooting

Initial bus setup does not enable internal pull-ups; use board or module pull-ups and match the device register-address width.
