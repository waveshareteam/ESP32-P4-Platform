# Audio Playback

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Generate a melody and play it through the ES8311 output codec.

## Hardware Required

An ESP32-P4 board with PSRAM, a USB data cable and the Platform ES8311 audio path and a connected speaker.

## How to Run

Follow the [Arduino setup](../../README.md), open `Audio_Playback.ino` and upload with the board options defined there.

## Expected Result

The serial monitor prints ES8311 playback ready and the speaker repeats the melody.

## Troubleshooting

The sketch uses the fixed Platform codec and amplifier wiring; playback stops if codec initialization fails.
