# 系统监视器

[English](README.md) | [简体中文](README_ZH.md)

## 功能

提供周期性内存采样、串口控制台和可持久化的采样设置。

## 硬件需求

ESP32-P4 开发板和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## 预期现象

monitor> 控制台支持 help、info、heap 和 period <ms>，周期性输出运行时间和内存状态。

## 常见问题

采样间隔保存到 NVS，复位后仍会保留。
