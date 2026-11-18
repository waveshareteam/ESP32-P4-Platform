# LVGL v9

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Run the bundled LVGL v9 widgets demo with display and touch support.

## Hardware Required

An ESP32-P4 board with PSRAM, a USB data cable and a compatible DSI touch display.

## How to Run

Follow the [Arduino setup](../../README.md), open `LVGLV9_Arduino.ino` and upload with the board options defined there. Select the connected panel with `CURRENT_SCREEN` before uploading.

## Expected Result

The display runs the LVGL widgets UI and responds to available touch input.

## Troubleshooting

Use the bundled lvgl and lv_conf.h together; GT911 input is unavailable if neither probed address responds.
