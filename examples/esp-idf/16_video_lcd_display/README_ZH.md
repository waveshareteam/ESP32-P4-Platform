# 摄像头画面显示

[English](README.md) | [简体中文](README_ZH.md)

## 功能

采集 MIPI-CSI 图像，经 PPA 处理后通过 BSP 显示到 LCD。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、兼容 DSI 显示屏、受支持的 MIPI-CSI 摄像头和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 menuconfig 中选择 LCD、摄像头传感器和输出格式。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

LCD 显示实时摄像头画面，串口输出所选视频格式。

## 常见问题

摄像头缓冲区需要 PSRAM；默认选择 OV5647 RAW8 800 x 1280，使用其他传感器时须匹配配置。
