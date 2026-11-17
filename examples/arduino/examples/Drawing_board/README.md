# Drawing Board

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Draw on the display using GT911 touch input.

## Hardware Required

An ESP32-P4 board with PSRAM, a USB data cable and a compatible DSI display with GT911 touch.

## How to Run

Follow the [Arduino setup](../../README.md), open `Drawing_board.ino` and upload with the board options defined there. Select the connected panel with `CURRENT_SCREEN` before uploading.

## Expected Result

Touching the drawing surface paints on the screen.

## Troubleshooting

Touch is polled after probing 0x5D and 0x14; if neither address responds, the sketch continues without touch.
