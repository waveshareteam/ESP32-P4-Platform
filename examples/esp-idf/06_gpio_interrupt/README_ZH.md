# GPIO 中断

[English](README.md) | [简体中文](README_ZH.md)

## 功能

将 GPIO 边沿事件从中断传递到任务，并进行软件消抖。

## 硬件需求

ESP32-P4 开发板、USB 数据线，以及连接到配置输入的按键或跳线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。在 Example Configuration 中选择输入 GPIO 和消抖时间。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

按下和释放输入时，串口输出电平和时间信息。

## 常见问题

输入启用上拉并检测双边沿，请按按键情况设置消抖间隔。
