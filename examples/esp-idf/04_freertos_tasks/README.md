# FreeRTOS Tasks

[中文版本](README_ZH.md)

Beginner-friendly FreeRTOS example with two tasks and one queue.

This is a good next step after `00_board_check` and `02_HelloWorld` because it
shows the task model used by most real ESP-IDF applications.

## Difficulty

Beginner to intermediate.

## Hardware Required

- One ESP32-P4 board.
- USB cable.

No external peripheral is required.

## Build and Flash

```bash
cd examples/esp-idf/04_freertos_tasks
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

## Expected Output

The producer task sends a message once per second. The consumer task receives
and prints it:

```text
producer sent message 0
consumer received message 0 at 1234 ms
```

## What to Reuse

- `xTaskCreate()` for creating tasks.
- `xQueueCreate()`, `xQueueSend()`, and `xQueueReceive()` for communication.
- `vTaskDelay()` for periodic work.
