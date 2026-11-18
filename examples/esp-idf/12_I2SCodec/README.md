# I2S Codec

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Play the embedded canon.pcm sample or run the configured microphone echo path through an audio codec.

## Hardware Required

An ESP32-P4 board with a supported codec, a speaker or headphones, and a microphone for echo mode.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Select music or echo mode and configure volume, microphone gain and the matching BSP or codec path.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The log reports successful I2S and ES8311 initialization, followed by playback or echo.

## Troubleshooting

Echo mode is available only in the non-BSP configuration; keep the bundled canon.pcm file with the project.
