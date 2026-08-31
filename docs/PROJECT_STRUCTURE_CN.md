# 项目结构

[English Version](./PROJECT_STRUCTURE.md)

本仓库组织为一组示例，而不是一个单体固件应用。

## 顶层目录

| 路径 | 用途 |
| --- | --- |
| `README.md` | 项目概览、快速开始、开发板支持和兼容性矩阵 |
| `config/` | 共享 ESP-IDF 配置 overlay，包括 ESP32-P4 芯片版本 profile |
| `examples/` | ESP-IDF 和 Arduino 示例 |
| `docs/` | 仓库级文档 |
| `.github/` | issue 和 pull request 模板 |
| `CONTRIBUTING.md` / `CONTRIBUTING_CN.md` | 贡献工作流 |
| `SUPPORT.md` / `SUPPORT_CN.md` | 支持渠道 |
| `SECURITY.md` / `SECURITY_CN.md` | 可执行的私密漏洞报告策略 |
| `THIRD_PARTY_CN.md` | 第三方代码清单 |
| `LICENSE.txt` | Apache-2.0 license |

## ESP-IDF 示例

`examples/esp-idf/` 下的每个目录都应可以独立构建。常见文件包括：

| 文件或目录 | 用途 |
| --- | --- |
| `CMakeLists.txt` | ESP-IDF 工程入口 |
| `main/` | 应用源代码 |
| `main/idf_component.yml` | 托管组件依赖（需要时） |
| `components/` | 示例本地组件 |
| `sdkconfig.defaults` | 提交到仓库的默认配置 |
| `sdkconfig.ci*` | 可选的 CI 配置 overlay |
| `partitions.csv` | 示例专属 partition table |
| `README.md` | 示例专属设置、构建和硬件说明 |

生成目录（例如 `build/`、`managed_components/`、`dependencies.lock`）以及本地 `sdkconfig` 文件不应提交。跨示例生效的芯片版本选择应使用 `config/` 中共享的 overlay，而不是复制到每个 `sdkconfig.defaults` 文件中。

## Arduino 示例

Arduino sketch 位于 `examples/arduino/examples/` 下。内置库位于 `examples/arduino/libraries/`，这样 sketch 可以用更少的手动依赖步骤打开和测试。

添加 Arduino 内容时：

- 将 sketch 添加到 `examples/arduino/examples/<ExampleName>/`。
- 更新 `examples/README.md` 和 `examples/README_CN.md`。
- 当所需 Arduino core 或内置库版本变化时，更新
  `examples/arduino/README.md` 和 `examples/arduino/README_CN.md`。
- 在 sketch README 或注释中记录开发板引脚假设。

## 文档期望

每个面向硬件的新示例都应记录：

- 支持的开发板。
- 所需外设、模块、线缆和跳线。
- ESP-IDF 或 Arduino-ESP32 版本期望。
- 必需的 `menuconfig` 选项或 Arduino board settings。
- 构建、烧录和 monitor 命令。
- 已知限制和故障排查说明。

示例覆盖范围和建议新增项请参见 [示例路线图](EXAMPLE_ROADMAP_CN.md)。
