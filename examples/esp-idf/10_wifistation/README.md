# Wi-Fi Station

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Connect through ESP-Hosted remote Wi-Fi and obtain an address from an access point.

## Hardware Required

An ESP32-P4 board with a configured ESP32-C6 Wi-Fi path, a 2.4 GHz access point and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Set the SSID, password, authentication threshold and remote Wi-Fi transport in menuconfig.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The serial log shows association attempts followed by the assigned IP address.

## Troubleshooting

Use C6 slave firmware compatible with the resolved host components; the application logs configured credentials, so remove them before sharing logs.
