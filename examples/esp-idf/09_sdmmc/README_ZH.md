# SDMMC

[English](README.md) | [简体中文](README_ZH.md)

## 功能

挂载 SD 卡，写入、重命名并读回测试文件，最后卸载存储卡。

## 硬件需求

带 SDMMC 卡槽的 ESP32-P4 开发板、SD 卡和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 SD/MMC Example Configuration 中核对总线宽度、GPIO、格式化和 SD 供电设置。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

串口输出存储卡信息，以及 hello.txt、foo.txt 和 nihao.txt 的文件操作结果。

## 常见问题

格式化选项会清除存储卡数据；启用前请核对总线宽度和板级 SD 供电配置。
