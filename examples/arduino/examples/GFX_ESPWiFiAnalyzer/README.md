# Wi-Fi Analyzer

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Scan nearby Wi-Fi networks and visualize signal levels.

## Hardware Required

An ESP32-P4 board with PSRAM, a USB data cable and a compatible DSI display and the ESP32-C6 Wi-Fi path.

## How to Run

Follow the [Arduino setup](../../README.md), open `GFX_ESPWiFiAnalyzer.ino` and upload with the board options defined there. Select the connected panel with `CURRENT_SCREEN` before uploading.

## Expected Result

The display lists detected networks and their signal levels.

## Troubleshooting

Wi-Fi initialization and failed scans are retried; the ESP32-C6 path must be available.
