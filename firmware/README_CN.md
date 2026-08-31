# 固件

[English Version](README.md)

本目录包含 ESP32-P4 Platform 出厂固件源码和预编译出厂固件包。

## 目录

- `brookesia/`：ESP-IDF 固件工程。
- `factory_firmware/`：预编译出厂固件包。

## 默认出厂固件

默认预编译固件为：

- `factory_firmware/ESP32-P4-Platform_Factory_Firmware_10.1-DSI-TOUCH-A_rev3.x-or-later`

该固件面向使用 16 MB Flash 的 ESP32-P4 rev3.x 或更新硬件。目录中也提供现有
7 英寸配置使用的 7-DSI-TOUCH-A 构建。

## 构建

使用 ESP-IDF v5.5 或更新版本：

```sh
cd firmware/brookesia
idf.py set-target esp32p4
idf.py build
```

固件工程从 ESP-IDF Component Registry 解析 LVGL 和
`waveshare/esp32_p4_platform`。

示例 CI 工作流不会构建这个维护固件工程，也不会重新生成预编译出厂包。现有
BIN 和 ZIP 文件属于发布制品；只有在明确的固件发布和校验值复核流程中才应更新。

独立的 firmware/Brookesia 产品 job 会构建 rev1_3 和 rev3_x，并发布临时的
32 MiB CI artifact。这些 profile 专属 artifact 不是出厂固件，也不会修改已提交到
factory_firmware/ 的不可变 rev3.x-or-later 16 MiB package。
