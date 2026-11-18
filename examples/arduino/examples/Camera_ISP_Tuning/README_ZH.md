# 摄像头 ISP 调节

[English](README.md) | [简体中文](README_ZH.md)

## 功能

预览 OV5647 视频，并通过非阻塞串口命令调整传感器设置。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、USB 数据线，以及兼容的 DSI 显示屏和 OV5647 摄像头。

## 怎么运行

按 [Arduino 环境说明](../../README_ZH.md)完成配置，打开 `Camera_ISP_Tuning.ino` 并使用其中的开发板选项上传。上传前通过 `CURRENT_SCREEN` 选择连接的屏幕。

## 预期现象

实时画面叠加当前增益、曝光和 AE 目标值；g/e/a/v/h/t 命令调整设置，s 输出设置。

## 常见问题

摄像头控制复用显示屏 I2C 总线，不受传感器支持的设置会被拒绝。
