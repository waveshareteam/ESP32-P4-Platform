# FreeRTOS 任务

[English Version](./README.md)

这是一个适合初学者的 FreeRTOS 示例，包含两个任务和一个队列。

它适合作为 `00_board_check` 和 `02_HelloWorld` 之后的下一步，因为它展示了大多数真实 ESP-IDF 应用都会使用的任务模型。

## 难度

入门到中级。

## 硬件要求

- 一块 ESP32-P4 开发板。
- USB 线。

不需要外接外设。

## 构建和烧录

```bash
cd examples/esp-idf/04_freertos_tasks
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

## 预期输出

生产者任务每秒发送一条消息。消费者任务接收并打印它：

```text
producer sent message 0
consumer received message 0 at 1234 ms
```

## 可复用内容

- 使用 `xTaskCreate()` 创建任务。
- 使用 `xQueueCreate()`、`xQueueSend()` 和 `xQueueReceive()` 进行任务间通信。
- 使用 `vTaskDelay()` 执行周期性工作。
