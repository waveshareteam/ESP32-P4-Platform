# SD Card

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Mount a four-bit SDMMC card, write a test file and read it back.

## Hardware Required

An ESP32-P4 board with PSRAM, a USB data cable and an inserted FAT32 microSD card in the supported SDMMC slot.

## How to Run

Follow the [Arduino setup](../../README.md), open `SD_Card.ino` and upload with the board options defined there.

## Expected Result

The serial monitor prints the card size and the contents of platform_sd_card.txt.

## Troubleshooting

The sketch uses the Platform SDMMC wiring and returns from setup if mounting fails.
