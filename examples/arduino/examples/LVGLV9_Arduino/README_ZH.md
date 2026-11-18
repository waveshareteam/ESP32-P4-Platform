# LVGL v9

[English](README.md) | [简体中文](README_ZH.md)

## 功能

运行随仓库提供的 LVGL v9 widgets 示例，并支持显示和触摸。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、USB 数据线，以及兼容的 DSI 触摸屏。

## 怎么运行

按 [Arduino 环境说明](../../README_ZH.md)完成配置，打开 `LVGLV9_Arduino.ino` 并使用其中的开发板选项上传。上传前通过 `CURRENT_SCREEN` 选择连接的屏幕。

## 预期现象

显示屏运行 LVGL widgets 界面，并响应可用的触摸输入。

## 常见问题

请配套使用仓库中的 lvgl 和 lv_conf.h；GT911 的两个探测地址均无响应时无法使用触摸。
