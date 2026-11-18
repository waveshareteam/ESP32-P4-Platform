# 出厂演示源码

[English](README.md) | [简体中文](README_ZH.md)

## 功能

在直角竖屏运行参考工程的 Brookesia 图标桌面。计算器、画板、频谱、音乐、视频和设置沿用旋钮出厂工程的界面布局，并按竖屏尺寸调整。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板，以及所选应用需要的显示屏、编解码器、摄像头和存储设备。

## 怎么运行

使用 [product.json](../../.github/product.json) 中指定的出厂工程 ESP-IDF 版本，在 menuconfig 选择 LCD 和实际 Flash 容量。默认配置为 rev3_x 芯片、800×1280 的 10.1-DSI-TOUCH-A 和 32 MB Flash。连接 720×1280 的 7-DSI-TOUCH-A 时，在 BSP 配置中选择对应面板。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

从屏幕底边向上划可返回桌面。在 Brookesia 页面中，上划后停留可打开最近任务；设置和曲目列表的子页可点带返回箭头的标题回到上一级。

| 启动器预览 | 亮度调节预览 |
| --- | --- |
| ![800×1280 出厂主页](../../docs/images/factory-home-800x1280.png) | ![720×1280 亮度调节](../../docs/images/factory-display-720x1280.png) |

## 常见问题

请保留 `spiffs/music/`：CMake 会将其中的音频打包到 storage 分区。Wi-Fi 需要与主机组件匹配的 C6 从机固件，参见 [P4 + C6 说明](../../docs/P4_C6_HOSTED_WIFI_ZH.md)。
