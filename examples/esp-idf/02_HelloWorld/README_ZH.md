# Hello World

[English](README.md) | [简体中文](README_ZH.md)

## 功能

运行 ESP-IDF Hello World 程序，输出芯片和内存信息。

## 硬件需求

ESP32-P4 开发板和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## 预期现象

串口输出 Hello world!、内存统计和重启倒计时。

## 常见问题

倒计时结束后重启是程序的预期行为。
