# Simple Video Server

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Serve a browser camera UI, snapshots and MJPEG streams using esp_video and HTTP servers.

## Hardware Required

An ESP32-P4 board with PSRAM, a supported camera, a configured Wi-Fi or Ethernet path and a browser on the same network.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Configure the camera and network in menuconfig; frontend changes follow the [frontend guide](frontend/README.md).

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

Open http://esp-web.local or the address printed by the monitor; the UI uses port 80 and streams begin on port 81.

## Troubleshooting

Keep frontend/gzipped with the project because its files are embedded in the image; configure the actual board and camera instead of assuming the default Function-EV-Board profile matches.
