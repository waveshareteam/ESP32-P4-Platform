# Wi-Fi 分析仪

[English](README.md) | [简体中文](README_ZH.md)

## 功能

扫描附近的 Wi-Fi 网络并显示信号强度。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、USB 数据线，以及兼容 DSI 显示屏和 ESP32-C6 Wi-Fi 通路。

## 怎么运行

按 [Arduino 环境说明](../../README_ZH.md)完成配置，打开 `GFX_ESPWiFiAnalyzer.ino` 并使用其中的开发板选项上传。上传前通过 `CURRENT_SCREEN` 选择连接的屏幕。

## 预期现象

显示屏列出检测到的网络及信号强度。

## 常见问题

Wi-Fi 初始化或扫描失败后会重试，需要可用的 ESP32-C6 通路。
