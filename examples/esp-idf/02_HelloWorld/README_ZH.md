# Hello World

[English Version](./README.md)

面向 ESP32-P4 的经典 ESP-IDF hello world 示例。

该示例会打印芯片信息、Flash 大小和堆信息，然后倒计时重启。如果你是第一次使用这块开发板，请先运行 [00_board_check](../00_board_check/)，因为它会持续运行，更容易在串口监视器中观察。

## 难度

入门。

## 硬件要求

- 一块 ESP32-P4 开发板。
- USB 线。

不需要外接外设。

## 构建和烧录

```bash
cd examples/esp-idf/02_HelloWorld
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为你的串口。

## 预期输出

串口监视器应打印 `Hello world!`、芯片信息、Flash 大小、最小堆大小以及重启倒计时。

## 说明

- 该示例会在倒计时结束后主动重启。
- 可在 `00_board_check` 之后作为基础 ESP-IDF 冒烟测试。
