# 创建工程

[English](README.md) | [简体中文](README_ZH.md)

## 功能

提供应用入口为空的最小 ESP-IDF 工程。

## 硬件需求

ESP32-P4 开发板和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## 预期现象

工程可以构建并启动，空的 app_main() 不输出应用日志。

## 常见问题

需要运行日志时请使用相邻的 00_board_check 示例。
