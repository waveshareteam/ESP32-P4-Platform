# 音频播放

[English](README.md) | [简体中文](README_ZH.md)

## 功能

生成旋律并通过 ES8311 输出编解码器播放。

## 硬件需求

带 PSRAM 的 ESP32-P4 开发板、USB 数据线，以及Platform ES8311 音频通路和连接的扬声器。

## 怎么运行

按 [Arduino 环境说明](../../README_ZH.md)完成配置，打开 `Audio_Playback.ino` 并使用其中的开发板选项上传。

## 预期现象

串口显示 ES8311 playback ready，扬声器循环播放旋律。

## 常见问题

草图使用固定的 Platform 编解码器和功放接线；编解码器初始化失败时不会播放。
