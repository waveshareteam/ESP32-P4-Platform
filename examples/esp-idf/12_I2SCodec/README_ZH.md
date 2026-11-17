# I2S 音频编解码器

[English](README.md) | [简体中文](README_ZH.md)

## 功能

通过音频编解码器播放内嵌的 canon.pcm，或运行配置的麦克风回放通路。

## 硬件需求

带兼容编解码器的 ESP32-P4 开发板、扬声器或耳机；回放模式还需麦克风。

## 怎么运行

使用[仓库首页](../../../README_ZH.md)列出的 ESP-IDF 版本，在本目录运行以下命令。选择音乐或回放模式，并配置音量、麦克风增益和匹配的 BSP 或编解码器通路。

```bash
idf.py set-target esp32p4
idf.py menuconfig
idf.py build flash monitor
```

## 预期现象

日志显示 I2S 和 ES8311 初始化成功，随后开始播放或麦克风回放。

## 常见问题

麦克风回放仅在非 BSP 配置下可选；工程需要保留 canon.pcm 文件。
