# 托管组件与本地组件策略

[English Version](COMPONENTS.md)

本仓库使用 ESP-IDF Component Manager 管理可复用的上游依赖，并把板级或示例
专属 glue 保留在拥有它的示例旁。生成的 `managed_components/` 目录和
`dependencies.lock` 文件不提交到仓库。

## 托管版本策略

第一方 ESP32-P4 示例使用以下约束：

| 组件 | 约束 | 范围和原因 |
| --- | --- | --- |
| `waveshare/esp32_p4_platform` | `2.0.1` | 示例 12、13、14、16、18 使用的精确板级 BSP 版本；升级时须有意更新并验证两条 CI 版本通道 |
| `esp_new_jpeg` | `1.0.2` | 示例 17 使用的精确 JPEG helper 版本，避免无边界 major 升级 |
| `esp_video` | `>=2.2,<3.0` | 示例 16、17 使用的兼容 2.x API 通道 |
| `espressif/esp_wifi_remote` | IDF 5.x：`0.14.*`；IDF 6.x：`>=1.6,<2.0` | 让生成的 `esp_wifi` 接口与所选 ESP-IDF 系列保持匹配 |
| `espressif/esp_hosted` | IDF 5.x：`1.4.*`；IDF 6.x：`>=2.12,<3.0` | 示例 10、15 的直接依赖；host/slave 兼容性还须在硬件上检查 |

不要把这些约束替换为 `"*"`。变更版本时同步更新本文，并在合并前让完整
ESP-IDF Actions 矩阵用 ESP-IDF `v5.5.5` 和 `v6.0.2` 解析并构建每个工程。
Hosted 运行时相关变更还须完成
[P4 + C6 Hosted Wi-Fi](P4_C6_HOSTED_WIFI_CN.md) 中的硬件检查。

## 示例本地组件

以下目录是有意维护的本地组件，不是陈旧生成物：

| 路径 | 分类 |
| --- | --- |
| `examples/esp-idf/09_sdmmc/components/sd_card` | 示例 09 自有的 SD-card helper 和测试 I/O glue |
| `examples/esp-idf/11_ethernetbasic/components/ethernet_init` | 本地 Ethernet 初始化 helper；示例 15 通过 `override_path` 复用 |
| `examples/esp-idf/12_I2SCodec` | 没有本地 support 组件；直接调用托管的 `waveshare/esp32_p4_platform` 和 `esp_codec_dev` 组件 |
| `examples/esp-idf/14_lvgl_demo_v9/components/lvgl_demo_support` | LVGL demo 的产品 glue，适用时包含显示行为和本地 helper |
| `examples/esp-idf/17_simple_video_server/components/example_video_common` | 带 target 条件源文件的共享视频初始化/JPEG helper |
| `examples/esp-idf/18_esp_brookesia_phone/components` | Phone demo 使用的 Brookesia 应用栈，包含本地 override path |

原有本地 BSP 副本已移除。可复用的板级支持来自在线
`waveshare/esp32_p4_platform` `2.0.1` 托管组件；`lvgl_demo_support` 和
`product_support` 仍是本地产品 glue，不是替代 BSP。只有组件具备稳定公共 API、
独立测试、版本化发布责任和多个仓库消费者后，才应考虑移出示例。

## CI 覆盖边界

默认 ESP-IDF matrix 发现 `examples/esp-idf/` 下 20 个直属工程，并以
`esp32p4` 为 target 构建。示例 10 的 ESP32-C2 配置和示例 12 的 ESP32-S3
BSP 配置等 target 专属文件会保留，但不会由本 P4 产品 matrix 编译。因此绿色
matrix 只证明 P4 编译，不能证明这些附加 target 或硬件运行时行为。

`firmware/` 下的维护源码工程和不可变发布包不属于示例 matrix，必须使用独立
发布流程。
