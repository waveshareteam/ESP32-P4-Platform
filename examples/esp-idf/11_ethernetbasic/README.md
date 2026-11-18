# Ethernet Basic

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Initialize Ethernet and obtain an IP address through DHCP.

## Hardware Required

An Ethernet-capable ESP32-P4 board, an Ethernet cable, a DHCP network and a USB data cable.

## How to Run

Use an ESP-IDF version from the [repository README](../../../README.md) and run these commands in this directory. Match the PHY model, address, clock and GPIO settings to the board in menuconfig.

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## Expected Result

The log reports Ethernet startup, link-up and the DHCP address.

## Troubleshooting

A configured deinitialization timeout stops Ethernet after the selected interval; leave it at -1 for continuous operation.
