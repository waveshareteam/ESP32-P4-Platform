# SD 卡

[English](README.md) | [简体中文](README_ZH.md)

## 功能

挂载四线 SDMMC 存储卡，写入并读回测试文件。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、USB 数据线，以及插入兼容 SDMMC 卡槽的 FAT32 microSD 卡。

## 怎么运行

按 [Arduino 环境说明](../../README_ZH.md)完成配置，打开 `SD_Card.ino` 并使用其中的开发板选项上传。

## 预期现象

串口显示存储卡容量和 platform_sd_card.txt 的内容。

## 常见问题

草图使用 Platform 的 SDMMC 接线；挂载失败时会退出 setup。
