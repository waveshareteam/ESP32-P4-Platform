# ESP-Brookesia Phone

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在微雪显示 BSP 上运行 ESP-Brookesia 手机界面和已注册应用。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、兼容的 DSI 触摸屏和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 menuconfig 中选择连接的 LCD 并核对 ESP-Brookesia 选项。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

背光点亮，显示手机界面，状态栏时钟周期性更新。

## 常见问题

默认屏幕为 7 英寸竖屏；请在 BSP 菜单选择实际 LCD，并保持显示缓冲配置一致。
