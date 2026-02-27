# ESP32-P4 Arduino Supported

## arduino-esp32 core 

### Latest Stable Release:

[![Release Version](https://img.shields.io/github/release/espressif/arduino-esp32.svg)](https://github.com/espressif/arduino-esp32/releases/latest/)
[![Release Date](https://img.shields.io/github/release-date/espressif/arduino-esp32.svg)](https://github.com/espressif/arduino-esp32/releases/latest/)
[![Downloads](https://img.shields.io/github/downloads/espressif/arduino-esp32/latest/total.svg)](https://github.com/espressif/arduino-esp32/releases/latest/)

The current recommendation is to use [![Release Version](https://img.shields.io/github/release/espressif/arduino-esp32.svg)](https://github.com/espressif/arduino-esp32/releases/latest/)

### Documentation

You can use the [Arduino-ESP32 Online Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/) to get all information about this project.

## Other Dependencies

### [lvgl v9.3.0](https://github.com/lvgl/lvgl)

<p align="center">
  <img src="https://lvgl.io/github-assets/logo-colored.png" width=300px>
</p>

  <h1 align="center">Light and Versatile Graphics Library</h1>
  <br>
<div align="center">
  <img src="https://lvgl.io/github-assets/smartwatch-demo.gif">
  &nbsp;
  <img border="1px" src="https://lvgl.io/github-assets/widgets-demo.gif">
</div>

### [Arduino_GFX v1.6.0](https://github.com/moononournation/Arduino_GFX)

Arduino GFX provides the encapsulated ESP32-P4 MIPI DSI function

## Special points to pay attention

### I2C Drivers

In libraries, we provide a way to wrap i2c_master.h ourselves and then provide some basic functions. The main reason is that after the esp-idf update, arduino-esp32 v3.2.0 uses a new version of i2c_master driver also known as i2c driver_ng, which is not compatible with some older libraries, including but not limited to some sensor libraries, touch libraries, and extended IO libraries.