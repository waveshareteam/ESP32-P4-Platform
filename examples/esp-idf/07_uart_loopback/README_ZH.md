# UART 回环

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过配置的 UART 发送文本，并从 RX 接收回环数据。

## 硬件需求

ESP32-P4 开发板、USB 数据线，以及连接配置 TX 和 RX GPIO 的跳线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 Example Configuration 中核对 UART 端口、波特率和 TX/RX GPIO。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

连接 TX 和 RX 后，串口监视器显示接收到的文本。

## 常见问题

回环 UART 与日志控制台独立；未接跳线时会输出接收超时提示。
