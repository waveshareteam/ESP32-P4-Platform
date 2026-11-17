# NVS 计数器

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在 NVS 中保存可跨重启保留的启动计数。

## 硬件需求

ESP32-P4 开发板和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## 预期现象

每次复位后，串口显示的 Saved boot count 都会增加。

## 常见问题

运行 idf.py erase-flash 会清除计数器及其他 Flash 数据。
