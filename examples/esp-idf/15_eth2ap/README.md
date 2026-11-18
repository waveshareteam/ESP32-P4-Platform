# Ethernet to Wi-Fi AP

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Forward packets between Ethernet and a remote Wi-Fi SoftAP.

## Hardware Required

An ESP32-P4 board with Ethernet and an ESP32-C6 Wi-Fi path, an Ethernet network and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Match the Ethernet PHY configuration and set the SoftAP name, password and channel in menuconfig.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The SoftAP starts when Ethernet links up, stops on link-down and forwards connected-client traffic.

## Troubleshooting

The C6 slave firmware must match the selected host stack; verify Ethernet and Wi-Fi separately before testing forwarding.
