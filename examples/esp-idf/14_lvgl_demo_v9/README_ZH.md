# LVGL v9 示例

[English](README.md) | [简体中文](README_ZH.md)

## 功能

使用微雪显示 BSP 和本地 LVGL 支持组件运行 widgets 示例。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、兼容的 DSI 显示屏，可选配触摸输入。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 BSP 菜单选择连接的屏幕，并使用工程提供的 LVGL v9 配置。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

显示屏运行 LVGL widgets 界面并显示性能监视信息。

## 常见问题

程序使用三缓冲局部防撕裂模式；请在 BSP 中选择实际屏幕，并保持显示缓冲配置一致。
