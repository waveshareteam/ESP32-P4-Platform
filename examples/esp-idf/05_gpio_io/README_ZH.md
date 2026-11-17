# GPIO 输入输出

[English](README.md) | [简体中文](README_ZH.md)

## 功能

每秒切换一次输出电平，并读取输入电平。

## 硬件需求

ESP32-P4 开发板、USB 数据线，可选用跳线连接配置的输入和输出。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 Example Configuration 中选择未被板载外设占用的输入和输出 GPIO。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

串口显示输出和输入电平；连接回环跳线后，输入跟随输出变化。

## 常见问题

输入默认启用上拉，未接跳线时可能保持高电平。
