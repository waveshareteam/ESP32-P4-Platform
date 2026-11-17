# FreeRTOS 任务

[English](README.md) | [简体中文](README_ZH.md)

## 功能

创建生产者和消费者任务，通过队列传递消息。

## 硬件需求

ESP32-P4 开发板和 USB 数据线。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。

```bash
idf.py set-target esp32p4
idf.py build flash monitor
```

## 预期现象

生产者每秒发送一条带序号的消息，消费者输出接收到的消息。

## 常见问题

暂无。
