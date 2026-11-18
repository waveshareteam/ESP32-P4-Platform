# GPIO Input and Output

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Toggle an output once per second and read an input.

## Hardware Required

An ESP32-P4 board, a USB data cable and an optional jumper between the configured input and output.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Select unused input and output GPIOs in Example Configuration.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The monitor prints output and input levels; a loopback jumper makes the input follow the output.

## Troubleshooting

The input has a pull-up enabled and may stay high without the jumper.
