# NVS 计数器

[English Version](./README.md)

这是一个适合初学者的示例，用 NVS flash 保存启动计数。

NVS 通常用于保存少量设置，例如校准值、设备名称、用户偏好或配网状态。

## 难度

入门。

## 硬件要求

- 一块 ESP32-P4 开发板。
- USB 线。

不需要外接外设。

## 构建和烧录

```bash
cd examples/esp-idf/03_nvs_counter
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

## 预期输出

串口监视器会打印已保存的启动次数：

```text
Saved boot count: 1
Press reset. The number should increase after each boot.
```

按下复位键后，计数应递增。

## 重置计数器

烧录前擦除 flash：

```bash
idf.py -p PORT erase-flash flash monitor
```

## 可复用内容

- `nvs_flash_init()` 初始化和恢复流程。
- `nvs_open()`、`nvs_get_u32()`、`nvs_set_u32()` 和 `nvs_commit()` 流程。
- 持久保存简单设置的模式。
