# 画板

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过 GT911 触摸输入在显示屏上绘图。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、USB 数据线，以及带 GT911 触摸的兼容 DSI 显示屏。

## 怎么运行

按 [Arduino 环境说明](../../README_ZH.md)完成配置，打开 `Drawing_board.ino` 并使用其中的开发板选项上传。上传前通过 `CURRENT_SCREEN` 选择连接的屏幕。

## 预期现象

触摸画板区域时，屏幕绘制对应笔迹。

## 常见问题

程序依次探测 0x5D 和 0x14 后轮询触摸；两个地址都不响应时，程序继续运行但没有触摸输入。
