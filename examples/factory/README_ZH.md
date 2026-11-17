# 出厂演示源码

[English](README.md) | [简体中文](README_ZH.md)

## 功能

运行 ESP-Brookesia 手机界面，提供计算器、画板、频谱、音乐、摄像头、视频和设置应用。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板，以及所选应用需要的显示屏、编解码器、摄像头和存储设备。

## 怎么运行

使用 [product.json](../../.github/product.json) 中指定的出厂工程 ESP-IDF 版本，在 menuconfig 选择实际 LCD 后于本目录构建。现有默认配置选择 rev3_x 芯片、10.1 英寸 800 x 1280 显示屏和 32 MB Flash。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

显示手机启动器，并可进入已安装的演示应用。

## 常见问题

请保留 `spiffs/music/`：CMake 会将其中的音频打包到 storage 分区。较早芯片使用独立的 legacy 默认配置，参见[修订版本指南](../../docs/ESP32P4_REVISION_CONFIG_ZH.md)。
