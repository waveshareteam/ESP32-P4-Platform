# 更新日志

[English](CHANGELOG.md)

本文件记录仓库级别的重要变更。

本项目目前尚未发布版本化发行版。在引入发行版之前，请使用带日期的条目记录面向用户的变更。

## 未发布

- 将 ESP-IDF 示例和 Arduino CI 默认切换到 ESP32-P4 v3.x profile，同时保留
  显式的 pre-v3/v1.3 构建 profile。
- Arduino 轮询示例在不指定 INT 或 RST GPIO 的情况下依次探测 GT911 的 `0x5D`
  和 `0x14` 地址，并在触摸不可用时安全继续运行。
- 新增 Arduino 摄像头预览、摄像头 ISP 调校、SD 卡和 ES8311 音频播放示例，
  同步提供中英文构建与 HIL 指引。
- 添加开源项目文档、贡献指南、安全策略、issue 模板、pull request 模板和示例索引。
- 添加面向初学者的快速开始、故障排查、示例指南和示例路线图文档。
- 添加 `00_board_check` ESP-IDF 示例，作为无需额外硬件的首次运行验证工程。
- 添加 `03_nvs_counter`、`04_freertos_tasks`、`05_gpio_io`、`06_gpio_interrupt`、`07_uart_loopback` 和 `19_app_skeleton`，作为从初学到高级的 ESP-IDF 示例。
- 将 ESP-IDF 示例重新排序为从基础验证到集成应用结构的 20 个演示学习路径。
