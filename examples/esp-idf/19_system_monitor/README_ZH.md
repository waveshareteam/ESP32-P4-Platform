# System Monitor

[English Version](./README.md)

面向高级用户的串口诊断和运行时监视器。

该 demo 适合作为产品 bring-up 或现场调试的起点。它组合了前面几个示例中的模式：

- NVS 初始化和持久设置。
- 默认 ESP-IDF event loop。
- FreeRTOS 队列和 worker tasks。
- 周期性 heap 和 PSRAM 采样。
- 交互式串口控制台命令。

## 难度

高级。

## 硬件要求

- 一块 ESP32-P4 开发板。
- USB 线。

不需要外接外设。

## 构建和烧录

```bash
cd examples/esp-idf/19_system_monitor
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

## 预期输出

监视器会打印周期性运行时采样：

```text
sample=0 uptime=1234ms heap=123456 min_heap=120000 psram=8123456 period=2000ms
```

串口控制台提示符为：

```text
monitor>
```

## 控制台命令

| 命令 | 用途 |
| --- | --- |
| `help` | 列出可用命令 |
| `info` | 打印 target、芯片版本、flash、PSRAM 和采样设置 |
| `heap` | 打印当前 heap 和 PSRAM 使用情况 |
| `period <ms>` | 保存新的采样间隔到 NVS |

示例：

```text
monitor> period 1000
sample period saved: 1000 ms
```

## 为什么有这个示例

这是一个面向真实项目的实用应用外壳。你可以在保持统一串口诊断入口的同时，扩展 Wi-Fi、以太网、显示、存储、传感器或生产测试命令。
