# 出厂固件

[English Version](README.md)

本目录包含预编译 ESP32-P4-Platform 出厂固件包。

默认固件包：

- `ESP32-P4-Platform_Factory_Firmware_10.1-DSI-TOUCH-A_rev3.x-or-later`

附加固件包：

- `ESP32-P4-Platform_Factory_Firmware_7-DSI-TOUCH-A_rev3.x-or-later`

每个固件包包含：

- 分段刷写镜像和 `flash_args`
- `merged-factory.bin`
- `flasher_args.json`
- `SHA256SUMS.txt`
- `README.txt`

下载 ZIP 后，请先使用保持二进制内容不变的归档工具解压到新目录，再进入 package
目录并在烧录前核验：

```sh
sha256sum -c SHA256SUMS.txt
```

Git checkout 中的展开目录只是便于浏览的镜像，并不是 release package 的校验权威；
Git 可能会归一化 `flash_args`、`flasher_args.json` 和 `README.txt` 等文本元数据的
换行。每个不可变 ZIP 内的 manifest 与 payload 才是校验边界。不要仅为了让工作树
文本校验值匹配而改写或重新打包已提交的 ZIP 或 BIN 文件。

更新已有设备时优先使用分段镜像，以免覆盖无关的数据分区。

在固件包目录中刷写分段镜像：

```sh
python -m esptool --chip esp32p4 -b 460800 --before default_reset --after hard_reset write_flash @flash_args
```

刷写 merged image：

```sh
python -m esptool --chip esp32p4 -b 460800 --before default_reset --after hard_reset write_flash 0x0 merged-factory.bin
```

这些不可变的 rev3.x-or-later 16 MiB package 不同于临时 32 MiB
firmware/Brookesia CI artifact。不要使用 CI artifact 替代出厂 package。
