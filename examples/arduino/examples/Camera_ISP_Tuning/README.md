# Camera ISP Tuning

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Preview OV5647 video and adjust sensor settings with non-blocking serial commands.

## Hardware Required

An ESP32-P4 board with PSRAM, a USB data cable and a compatible DSI display and an OV5647 camera.

## How to Run

Follow the [Arduino setup](../../README.md), open `Camera_ISP_Tuning.ino` and upload with the board options defined there. Select the connected panel with `CURRENT_SCREEN` before uploading.

## Expected Result

Live video includes the current gain, exposure and AE target; g/e/a/v/h/t commands adjust settings and s prints them.

## Troubleshooting

Camera control reuses the display I2C bus; unsupported sensor settings are rejected.
