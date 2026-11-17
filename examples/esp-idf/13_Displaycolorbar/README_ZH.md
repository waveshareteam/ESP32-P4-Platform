# 显示彩条

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过微雪 BSP 初始化显示屏，并启用硬件彩条发生器。

## 硬件需求

ESP32-P4 开发板、兼容的 DSI 显示屏和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 Board Support Package(ESP32-P4) 中选择连接的 LCD。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

背光点亮，LCD 显示竖向彩条。

## 常见问题

运行彩条测试前，请在 BSP 配置中选择实际连接的屏幕。
