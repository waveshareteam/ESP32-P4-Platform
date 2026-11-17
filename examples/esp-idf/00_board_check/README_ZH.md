# 开发板检查

[English](README.md) | [简体中文](README_ZH.md)

## 功能

输出芯片、Flash、PSRAM 和堆内存信息，并每五秒打印一次运行日志。

## 硬件需求

ESP32-P4 开发板和 USB 数据线，无需外接外设。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## 预期现象

串口监视器显示开发板检查标题、芯片修订版本和周期性运行日志。

## 常见问题

默认配置面向 v3.x 芯片；较早芯片请按修订版本指南显式选择 legacy 配置。参见[修订版本指南](../../../docs/ESP32P4_REVISION_CONFIG_ZH.md)。
