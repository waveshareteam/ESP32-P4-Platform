# 安全策略

[English Version](SECURITY.md)

## 私密报告渠道

不要在 GitHub issue、discussion、pull request、日志或截图中公开漏洞细节。

请通过 Waveshare 官方支持门户 <https://service.waveshare.com> 向 Waveshare
Support Team 提交私密工单。注明仓库为 `waveshareteam/ESP32-P4-Platform`，并将
请求标记为安全报告。本仓库当前未启用 GitHub private vulnerability reporting，
因此请使用支持门户，而不是公开 issue tracker。

只包含调查所需的信息：

- 受影响的示例、组件、固件包或配置路径。
- 清晰的问题描述、影响和受影响开发板/版本。
- 复现步骤或最小 proof of concept。
- 相关版本和 commit SHA。
- 问题或利用方式是否已经公开。

删除无关凭据、token、个人数据、私有网络信息、唯一设备标识和本地路径。如果
解释问题必须使用 secret，请先询问支持团队应如何安全传输。

## 范围与支持

本策略覆盖仓库维护的第一方代码和配置。若漏洞来自随附库或托管组件，可能还需
与该上游维护者协调报告。

默认分支接收当前修复。历史快照、发布制品和下游 fork 只有在明确说明时才受
支持。本文不承诺公开的响应或修复时限；请保留私密工单编号以便跟进。
