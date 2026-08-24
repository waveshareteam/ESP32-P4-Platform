# ESP32-P4 版本配置

[English Version](./ESP32P4_REVISION_CONFIG.md)

本仓库维护两个彼此不兼容的 ESP32-P4 产品 profile。它们由产品构建传入的版本
配置选择；不要在两个 profile 之间复用生成输出。

## 产品 Profile

| Profile | 芯片版本设置 | 目标 silicon | 说明 |
| --- | --- | --- | --- |
| `rev3_x`（默认） | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=n`；`CONFIG_ESP32P4_REV_MIN_300=y` | ESP32-P4 rev v3.0 及以后 | 量产 v3.x silicon 的默认配置。 |
| `rev1_3`（legacy，pre-v3） | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y`；`CONFIG_ESP32P4_REV_MIN_100=y` | 低于 v3.0 的 ESP32-P4，包括 v1.3 | ESP-IDF 没有官方的 `v1.3` 专用 Kconfig；`MIN_100` 覆盖整个 v1.x 系列。 |

默认 profile 是 `rev3_x`。`rev1_3` 仍保留为显式选择的 legacy/pre-v3 兼容
profile，并不表示 ESP-IDF Kconfig 能识别单独的 v1.3 stepping。

## 独立的构建输出

两个 profile 使用独立的 build 目录、生成的 `sdkconfig`、二进制文件和 CI
artifact，彼此不可互换。在本地 checkout 切换 profile 时，请使用独立 build
目录，或先移除旧的生成配置再重新构建；已有 `sdkconfig` 可能覆盖你本来要选择的
defaults。

请选择与 silicon 相符的 profile：v3.0 及以后使用 `rev3_x`；包括 v1.3 在内的
pre-v3 silicon 显式使用 `rev1_3`。产品 flasher 会探测 silicon revision，并拒绝
所选 profile 不匹配的情况。但 silicon 检查不能证明某个开发板的 PCB、显示、电源、
外设或其他电气兼容性。

Arduino-ESP32 `3.3.11` 对应的默认配置是 `ChipVariant=postv3`（v3.00+、400 MHz）；
显式 legacy 选择是 `ChipVariant=prev3`（pre-v3、360 MHz）。两种 Arduino 构建输出
不可互换，也不可与另一版本 profile 的 ESP-IDF `sdkconfig` 或二进制混用。

## 示例覆盖边界

ESP-IDF 示例 matrix 是一个默认 profile matrix：20 个直属工程使用 ESP-IDF
`v5.5.5` 和 `v6.0.2`，并使用 `rev3_x`。Arduino matrix 是 9 个 sketch，使用
Arduino-ESP32 `3.3.11` 和 `ChipVariant=postv3`；其分段诊断 package 因此只能作为
v3.00+ profile 的候选。示例不会按版本翻倍。维护的 firmware/Brookesia 产品 job
仍同时保留 `rev1_3` 和 `rev3_x`。

## 证据边界

编译成功或 CI job 通过只能证明所选软件配置能够编译，不能证明烧录、启动、显示、
触摸、摄像头、音频、网络或其他硬件在环行为。只有在准确的开发板和已连接硬件上
测试后，才能将某个 profile 视为已完成硬件验收。
