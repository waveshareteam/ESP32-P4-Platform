# 以太网转 Wi-Fi AP

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在以太网和远程 Wi-Fi SoftAP 之间转发数据包。

## 硬件需求

同时具有以太网和 ESP32-C6 Wi-Fi 通路的 ESP32-P4 开发板、以太网网络和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 menuconfig 中匹配以太网 PHY，并设置 SoftAP 名称、密码和信道。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

以太网连接时启动 SoftAP，断开时停止，并转发已连接客户端的数据。

## 常见问题

C6 从机固件须与主机组件匹配；测试转发前请分别确认以太网和 Wi-Fi 通路。
