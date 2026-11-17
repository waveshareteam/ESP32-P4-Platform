# ESP32-P4 + ESP32-C6 Hosted Wi-Fi

[English Version](P4_C6_HOSTED_WIFI.md)

ESP32-P4 没有原生 Wi-Fi 射频。Wi-Fi 示例可以通过 ESP-Hosted 把 ESP32-C6
作为协处理器，并由 `esp_wifi_remote` 暴露远端接口。P4 host 组件和 C6 slave
固件构成一个兼容性整体；P4 编译成功不能证明任意 C6 镜像都能工作。

## 仓库版本通道

| 范围 | Host stack | C6 固件边界 |
| --- | --- | --- |
| ESP-IDF 5.x 示例 10、15 | `esp_wifi_remote` `0.14.*`；`esp_hosted` `1.4.*` | 使用所解析 Hosted release 要求的 slave 固件 |
| ESP-IDF 6.x 示例 10、15 | `esp_wifi_remote` `>=1.6,<2.0`；`esp_hosted` `>=2.12,<3.0` | 使用所解析 Hosted release 要求的 slave 固件 |
| ESP-IDF 示例 17 | 使用相同的直接 `esp_wifi_remote` 通道；transport 依赖由所选组件图解析 | 必须一起验证解析后的组件图和 slave 镜像 |
| Arduino-ESP32 `3.3.11` | 使用所选 Arduino core 随附的依赖 | 不要独立覆盖某一个 Hosted 组件 |

不要只根据 registry 中的最高版本推断兼容性。ESP-IDF 系列、
`esp_wifi_remote`、ESP-Hosted host 和 C6 slave 固件必须作为一个经过验证的
组合进行匹配。

## CI 能证明什么

ESP-IDF matrix 证明每个直属 P4 示例使用默认 `rev3_x`/`MIN_300` 可在两条声明的 IDF
通道上编译。Arduino 工作流证明 9 个 sketch 使用默认 `ChipVariant=postv3`
（v3.00+、400 MHz）profile 编译，并验证分段 package 完整性。pre-v3/v1.3 需要显式
`rev1_3`/`MIN_100` 或 `prev3`（360 MHz），不同芯片版本系列的输出不可互换。两个工作流
都不会烧录 P4、烧录 C6、证明关闭 monitor 时的
冷启动、协商 SDIO 链路、连接 AP 或测试网络流量。

## 硬件验收

把 Hosted 变更判定为完成前：

1. 烧录与开发板芯片版本匹配的 P4 镜像。
2. 烧录该 host stack 明确要求的 C6 slave 镜像。
3. 采集新鲜 boot log，确认 Hosted SDIO 初始化和 C6 枚举成功，且没有反复重试
   或 reset storm。
4. 确认关联、DHCP、DNS 和双向网络流量。
5. 测试断开/重连和 warm reboot。
6. 记录开发板型号/版本、P4 和 C6 镜像版本、组件版本和实测结果。分享日志前
   删除凭据、唯一标识、私有网络信息和本地路径。
