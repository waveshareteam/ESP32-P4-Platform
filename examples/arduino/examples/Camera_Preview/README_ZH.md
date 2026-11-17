# 摄像头预览

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在显示屏上预览 OV5647 MIPI-CSI 摄像头画面。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、USB 数据线，以及兼容的 DSI 显示屏和 OV5647 摄像头。

## 怎么运行

按 [Arduino 环境说明](../../README_ZH.md)完成配置，打开 `Camera_Preview.ino` 并使用其中的开发板选项上传。上传前通过 `CURRENT_SCREEN` 选择连接的屏幕。

## 预期现象

显示屏显示实时 RGB565 摄像头画面。

## 常见问题

摄像头控制复用显示屏 I2C 总线，请勿另建摄像头控制总线。
