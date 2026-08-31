# 贡献指南

[English Version](CONTRIBUTING.md)

感谢你帮助改进 ESP32-P4 Platform 示例。

## 开始之前

- 先搜索现有 issue 和 pull request，避免重复工作。
- 选择能说明问题或改进的最小示例或文档范围。
- 涉及硬件时，尽可能记录准确的开发板名称和版本。

## 开发环境

### ESP-IDF

使用与所改示例匹配的 ESP-IDF 版本。CI 会用 ESP-IDF `v5.5.5` 和 `v6.0.2`
构建全部第一方 ESP32-P4 示例；个别示例可能在 README 和
`idf_component.yml` 中声明不同的最低版本或额外 target 配置。

典型流程：

```bash
cd examples/esp-idf/00_board_check
idf.py set-target esp32p4
idf.py build
idf.py -p PORT flash monitor
```

如果本地路径较长或位于较慢磁盘，可使用外部构建目录：

```bash
idf.py -B build-esp32p4 set-target esp32p4 build
```

### Arduino

修改 Arduino 内容前先阅读
[examples/arduino/README_CN.md](examples/arduino/README_CN.md)。在 pull request
说明中包含 Arduino-ESP32 core 版本和开发板选择。

## Pull Request 检查清单

提交 pull request 前：

- 构建发生变化的示例，或说明为什么未能在本地构建。
- 如果改动影响运行时行为，在真实硬件上运行示例。
- 命令、依赖、引脚或开发板支持变化时同步更新中英文 README。
- 不要提交生成文件，包括 `build/`、`sdkconfig`、`sdkconfig.old`、
  `managed_components/` 和 `dependencies.lock`。
- 每个 pull request 聚焦一个主题。

添加新示例时，使用[示例路线图](docs/EXAMPLE_ROADMAP_CN.md)选择有价值的
空白，并按[项目结构](docs/PROJECT_STRUCTURE_CN.md)组织文件。

## Commit 风格

使用简短、明确的 commit message，例如：

```text
fix(i2c): document pull-up requirements
docs(readme): add quick start commands
feat(display): add color bar panel option
```

## 报告问题

提交 issue 时请包含：

- 开发板名称和版本。
- 示例路径。
- ESP-IDF 或 Arduino-ESP32 版本。
- 主机操作系统。
- 完整构建错误或已脱敏串口日志。公开前删除凭据、token、私有网络信息、
  唯一设备标识和本地路径。
- 硬件连接相关时提供照片或接线说明。

## 行为准则

参与本项目须遵守 [CODE_OF_CONDUCT_CN.md](CODE_OF_CONDUCT_CN.md)。
