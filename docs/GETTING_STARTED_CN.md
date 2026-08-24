# 入门指南

[English Version](./GETTING_STARTED.md)

本指南面向首次使用的用户。如果你已经熟悉 ESP-IDF 或 Arduino，可以使用 [示例指南](EXAMPLES_GUIDE_CN.md) 直接跳到合适示例。

## 选择路径

| 你想要... | 从这里开始 |
| --- | --- |
| 确认开发板和工具链可以工作 | `examples/esp-idf/00_board_check` |
| 学习基础 ESP-IDF build 和 monitor 流程 | `examples/esp-idf/02_HelloWorld` |
| 从 Arduino 使用显示屏 | `examples/arduino/examples/HelloWorld` |
| bring up 一个硬件外设 | 从 [示例指南](EXAMPLES_GUIDE_CN.md) 选择 |
| 调试 build、flash 或 boot 问题 | [故障排查](TROUBLESHOOTING_CN.md) |

## 第一次运行 ESP-IDF

1. 安装 [CI 说明](CI_CN.md)中列出的 ESP-IDF 版本。CI 当前检查 `v5.5.5`
   和 `v6.0.2`；个别示例 README 可能声明额外约束。
2. 打开 ESP-IDF terminal。
3. 克隆本仓库。
4. 构建 board check 示例：

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
```

5. 默认 profile 是用于 v3.x silicon 的 `rev3_x`/`MIN_300`。包括 v1.3 在内的
   pre-v3 silicon 请显式选择 legacy `rev1_3`，它使用 `MIN_100` 和
   `SELECTS_REV_LESS_V3`。参阅 [ESP32-P4 版本配置](ESP32P4_REVISION_CONFIG_CN.md)；
   两个 profile 的生成 `sdkconfig` 和二进制不可复用。
6. 烧录并打开 monitor：

```bash
idf.py -p PORT flash monitor
```

将 `PORT` 替换为你的串口。

如果看到 board check banner 和周期性的 `alive` 日志，说明你的环境已经准备好。

## 第一次运行 Arduino

1. 安装 Arduino IDE 或 arduino-cli。
2. 安装兼容的 Arduino-ESP32 core。参见
   [examples/arduino/README_CN.md](../examples/arduino/README_CN.md)。
3. 在 Arduino board settings 中启用 PSRAM，并选择 `ChipVariant=postv3`
   （v3.00+、400 MHz）。只有 legacy pre-v3 开发板才选择
   `ChipVariant=prev3`（360 MHz）；两种构建输出不可互换。
4. 打开 `examples/arduino/examples/HelloWorld/HelloWorld.ino`。
5. 选择匹配的开发板和 upload port。
6. 上传 sketch。

本仓库中的大多数 Arduino 示例都偏向显示应用。它们需要兼容的 DSI display 配置和
PSRAM。同一份 [Arduino 指南](../examples/arduino/README_CN.md)还说明分段 CI
artifact、分段烧录，以及关闭串口监视器时必须执行的冷启动测试。打开 monitor
绝不是 sketch 启动的前提。

## 初学者检查清单

- 使用支持数据传输的 USB 线，而不是仅充电线。
- 确认开发板连接后串口会出现。
- 在运行需要 Wi-Fi、Ethernet、SD card、display、touch、audio 或 camera 硬件的示例之前，先从 `00_board_check` 开始。
- v3.x 芯片使用默认 `rev3_x`/`MIN_300`；包括 v1.3 在内的 v1.x 芯片显式使用
  legacy `rev1_3`/`MIN_100`。
- 阅读所选示例目录中的 README。
- 如果示例有 `menuconfig` 设置，请在构建前完成配置。

## 接下来阅读什么

- [示例指南](EXAMPLES_GUIDE_CN.md)：按难度和外设选择示例。
- [故障排查](TROUBLESHOOTING_CN.md)：常见设置和运行时问题。
- [项目结构](PROJECT_STRUCTURE_CN.md)：面向贡献者的仓库布局。
