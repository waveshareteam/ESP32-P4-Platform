# Camera Preview

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Preview OV5647 MIPI-CSI frames on the display.

## Hardware Required

An ESP32-P4 board with PSRAM, a USB data cable and a compatible DSI display and an OV5647 camera.

## How to Run

Follow the [Arduino setup](../../README.md), open `Camera_Preview.ino` and upload with the board options defined there. Select the connected panel with `CURRENT_SCREEN` before uploading.

## Expected Result

The display shows live RGB565 camera frames.

## Troubleshooting

Camera control reuses the display I2C bus; do not create a separate camera-control bus.
