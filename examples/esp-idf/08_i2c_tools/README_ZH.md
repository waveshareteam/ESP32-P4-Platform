# I2C 工具

[English](README.md) | [简体中文](README_ZH.md)

## 功能

提供串口控制台，用于扫描 I2C 设备和读写寄存器。

## 硬件需求

ESP32-P4 开发板、USB 数据线，可选配带适当上拉的 I2C 设备。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 Example Configuration 中配置 SDA、SCL 和可选的命令历史存储。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

i2c-tools> 提示符可执行 help、i2cdetect、i2cconfig、i2cget 和 i2cset。

## 常见问题

初始总线不启用内部上拉，请使用板载或模块上拉，并核对设备寄存器地址宽度。
