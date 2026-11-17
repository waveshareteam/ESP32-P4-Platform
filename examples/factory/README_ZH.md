# 出厂演示源码

[English](README.md) | [简体中文](README_ZH.md)

## 功能

运行面向 800×1280 直角竖屏的深色卡片界面，提供计算器、画板、频谱、音乐、摄像头、视频和设置应用。音乐页直接显示内置音频的文件名，并提供播放、暂停、切歌和曲目列表。

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

主页按三列展示应用，设置页使用蓝色控件和统一的返回导航；摄像头和视频保持全屏预览。下图为软件渲染的界面预览。

| 启动器 | 设置 |
| --- | --- |
| ![800×1280 出厂主页](../../docs/images/factory-home-800x1280.png) | ![800×1280 出厂设置](../../docs/images/factory-settings-800x1280.png) |

## 常见问题

请保留 `spiffs/music/`：CMake 会将其中的音频打包到 storage 分区。Wi-Fi 需要与主机组件匹配的 C6 从机固件，参见 [P4 + C6 说明](../../docs/P4_C6_HOSTED_WIFI_ZH.md)。
