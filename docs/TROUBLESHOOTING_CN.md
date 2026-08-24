# 故障排查

[English Version](./TROUBLESHOOTING.md)

从 `examples/esp-idf/00_board_check` 开始。它不需要外部硬件，可以帮助区分工具链问题和外设问题。

## 构建问题

### 找不到 `idf.py`

构建前先打开 ESP-IDF terminal，或 source ESP-IDF export script。然后运行：

```bash
idf.py --version
```

### 目标芯片错误

本仓库中的大多数示例目标芯片是 ESP32-P4。请在示例目录中运行：

```bash
idf.py set-target esp32p4
```

### ESP32-P4 芯片版本不匹配

早于 rev v3.0 的 ESP32-P4 芯片与 rev v3.0 或之后的芯片是互斥构建目标。如果镜像可以构建但无法在另一个 P4 版本系列上启动，请使用 [ESP32-P4 版本配置](ESP32P4_REVISION_CONFIG_CN.md) 中正确的 overlay 重新构建。

默认 profile 是面向 v3.0 及以后的 rev3_x（`MIN_300`）；包括 v1.3 在内的 pre-v3
silicon 使用显式 legacy rev1_3（`MIN_100` 和 `SELECTS_REV_LESS_V3`）。它们生成的
sdkconfig、build 目录和二进制文件彼此独立。重新构建前请参阅
[ESP32-P4 版本配置](ESP32P4_REVISION_CONFIG_CN.md)。

### 托管组件下载失败

一些示例会通过 `idf_component.yml` 获取依赖。请检查网络连接并重试构建。如果你位于代理后面，请先配置 ESP-IDF 和 Git proxy settings。

### VS Code plugin 构建失败

一些网络示例包含旧说明：通过 VS Code plugin 工作流构建时可能失败。请先使用 ESP-IDF terminal：

```bash
cd examples/esp-idf/10_wifistation
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
```

终端构建成功后，再对比 VS Code plugin 的环境、target 和配置。

## 烧录和监视器问题

### 串口缺失

- 使用支持数据传输的 USB 线。
- 在 Windows 上检查 Device Manager，或在 Linux/macOS 上检查 `/dev/tty*`。
- 关闭其他串口监视程序。
- 如果你的开发板需要手动 boot mode，请按下 reset 或 boot 按键。

### 烧录成功但 monitor 输出不可读

检查 monitor baud rate。使用以下命令时，ESP-IDF 默认值通常是正确的：

```bash
idf.py -p PORT flash monitor
```

仓库 Arduino profile 禁用了 `USB CDC On Boot`，`Serial` 会映射到 UART0。关闭
所有 monitor 时，应用也必须能正常冷启动；只在启动完成后再以 `115200` 打开开发板
的 UART0 monitor。如果显示或其他主要功能会等待 monitor，请把它作为启动缺陷报告，
并附上精确 FQBN 和 board options。参阅
[Arduino HIL 清单](../examples/arduino/README_CN.md#串口启动与硬件在环检查)。

## 运行时问题

### PSRAM 未初始化

许多 display、LVGL、camera 和 Arduino 示例需要 PSRAM。请用 `00_board_check` 确认 PSRAM，然后检查示例的 `sdkconfig.defaults` 和开发板设置。

### Wi-Fi 无法连接

- 确认开发板具备 Wi-Fi 支持，或已配置 Wi-Fi companion 路径。
- 运行 `idf.py menuconfig` 并设置 SSID/password。
- 检查接入点是否为 2.4 GHz 且可达。
- 查看串口日志中的认证、超时或重试消息。

### Ethernet link down

- 确认开发板具备 Ethernet 硬件。
- 检查 PHY model、PHY address、MDC/MDIO GPIO、reset GPIO 和 clock mode。
- 确认网线、交换机和 DHCP server。

### SD card 挂载失败

- 确认 card 已插入并已格式化。
- 如果接线或信号完整性不确定，尝试 1-line SDMMC mode。
- 除非你愿意擦除 card，否则不要启用格式化选项。

### 显示屏空白

- 确认已选择正确的 panel 和 `CURRENT_SCREEN` 或 BSP 配置。
- 确认已启用 PSRAM。
- 先检查 backlight control，再查看 panel init 日志。
- 在运行 LVGL demo 之前，先从简单 color bar 或 Arduino `HelloWorld` 开始。

### 触摸无响应

- 确认 touch controller model 和 I2C pins。
- Platform GT911 路径只使用轮询，不指定 INT 或 RST。
- 创建 touch driver 前，在共用 I2C bus 上先探测 `0x5D`、再探测 `0x14`，并以响应
  地址初始化。
- 两个地址都未响应时，保留安全的无触摸路径，不要假定固定地址或复制 LCD-5 接线。

## 寻求帮助

提交 issue 时，请包含：

- 开发板名称和版本。
- 示例路径。
- ESP-IDF 或 Arduino-ESP32 版本。
- 完整构建命令。
- 完整错误日志或串口输出。
- 涉及外部硬件时，提供照片或接线说明。
