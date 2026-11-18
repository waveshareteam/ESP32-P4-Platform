# 以太网基础

[English](README.md) | [简体中文](README_ZH.md)

## 功能

初始化以太网并通过 DHCP 获取 IP 地址。

## 硬件需求

支持以太网的 ESP32-P4 开发板、网线、DHCP 网络和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 menuconfig 中按开发板匹配 PHY 型号、地址、时钟和 GPIO 设置。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

日志显示以太网启动、链路连接和 DHCP 地址。

## 常见问题

配置反初始化超时时，程序会在指定时间后停止以太网；持续运行请保留 -1。
