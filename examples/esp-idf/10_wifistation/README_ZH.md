# Wi-Fi Station

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 ESP-Hosted 远程 Wi-Fi 连接热点并获取地址。

## 硬件需求

具备 ESP32-C6 Wi-Fi 通路的 ESP32-P4 开发板、2.4 GHz 热点和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 menuconfig 中设置 SSID、密码、认证阈值和远程 Wi-Fi 传输配置。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

串口显示连接过程和分配的 IP 地址。

## 常见问题

C6 从机固件须与解析的主机组件匹配；程序会记录配置的凭据，分享日志前请移除这些内容。
