# 视频服务器

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 esp_video 和 HTTP 服务提供浏览器摄像头界面、抓拍和 MJPEG 视频流。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、受支持的摄像头、配置好的 Wi-Fi 或以太网通路，以及同网段浏览器。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 menuconfig 中配置摄像头和网络；修改前端时请按[前端说明](frontend/README_ZH.md)重新生成资源。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

打开 http://esp-web.local 或串口显示的地址；界面使用端口 80，视频流从端口 81 开始。

## 常见问题

frontend/gzipped 会嵌入固件，必须随工程保留；请按实际板卡和摄像头调整默认的 Function-EV-Board 配置。
