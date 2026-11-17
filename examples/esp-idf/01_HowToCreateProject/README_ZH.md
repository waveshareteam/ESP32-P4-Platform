# 如何创建工程

[English Version](./README.md)

这是本仓库中最小的 ESP-IDF 工程结构。它适合用来理解在加入真实应用逻辑之前，一个工程至少需要哪些文件。

## 难度

入门。

## 硬件要求

- 一块 ESP32-P4 开发板。
- USB 线。

不需要外接外设。

## 构建

```bash
cd examples/esp-idf/01_HowToCreateProject
idf.py set-target esp32p4
idf.py build
```

这个示例的 `app_main()` 是空的，因此主要作为工程结构参考。如果想要更适合第一次运行的示例，请使用 [00_board_check](../00_board_check/)。

## 需要注意的文件

- `CMakeLists.txt`：工程入口。
- `main/CMakeLists.txt`：把源文件注册为 ESP-IDF 组件。
- `main/main.c`：应用入口。
