# GPIO 中断

[English Version](./README.md)

GPIO 中断和消抖示例。

这个示例是 [05_gpio_io](../05_gpio_io/) 之后的下一步。它演示如何在 ISR 中处理 GPIO 边沿、把事件传递到 FreeRTOS 队列，并在普通任务中处理事件。

## 难度

入门到中级。

## 硬件要求

- 一块 ESP32-P4 开发板。
- 按键、跳线或其他安全的 3.3 V 输入源。

## 默认引脚

| 信号 | 默认 GPIO |
| --- | --- |
| 输入 | GPIO3 |

在 `idf.py menuconfig` 中可以修改输入 GPIO 和消抖时间。

## 接线说明

默认启用了输入上拉。做简单测试时，可以把按键或跳线连接在 GPIO3 和 GND 之间。示例会在上升沿和下降沿都触发，因此按下和松开输入时日志都应更新。

输入只能使用 3.3 V 逻辑。不要把 GPIO 直接连接到 5 V 信号。

## 构建和烧录

```bash
cd examples/esp-idf/06_gpio_interrupt
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## 预期输出

当输入电平变化时，监视器会打印事件：

```text
gpio=3 level=0 time=1234 ms
gpio=3 level=1 time=1456 ms
```

## 可复用内容

- `gpio_install_isr_service()` 和 `gpio_isr_handler_add()`。
- 使用 `xQueueSendFromISR()` 把 ISR 事件传递到任务。
- 一个简单的软件消抖模式。
