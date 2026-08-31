# Factory Firmware

[中文版本](README_CN.md)

This directory contains prebuilt ESP32-P4-Platform factory firmware packages.

Default package:

- `ESP32-P4-Platform_Factory_Firmware_10.1-DSI-TOUCH-A_rev3.x-or-later`

Additional package:

- `ESP32-P4-Platform_Factory_Firmware_7-DSI-TOUCH-A_rev3.x-or-later`

Each package includes:

- individual flash images and `flash_args`
- `merged-factory.bin`
- `flasher_args.json`
- `SHA256SUMS.txt`
- `README.txt`

Verify `SHA256SUMS.txt` against the files extracted from the downloaded ZIP
before flashing. Extract into a new directory with a binary-preserving archive
tool, enter the package directory, and run:

```sh
sha256sum -c SHA256SUMS.txt
```

The expanded directories in a Git checkout are browsing mirrors, not the
checksum authority for the release packages: Git may normalize line endings in
text metadata such as `flash_args`, `flasher_args.json`, and `README.txt`. The
manifest and payload inside each immutable ZIP remain the verification
boundary. Do not rewrite or repackage the checked-in ZIP or BIN files merely to
make a working-tree text checksum match.

Prefer the split images when updating an existing device so unrelated data
partitions are not overwritten.

Flash separate images from inside a package directory:

```sh
python -m esptool --chip esp32p4 -b 460800 --before default_reset --after hard_reset write_flash @flash_args
```

Flash the merged image:

```sh
python -m esptool --chip esp32p4 -b 460800 --before default_reset --after hard_reset write_flash 0x0 merged-factory.bin
```

These immutable rev3.x-or-later 16 MiB packages are distinct from temporary
32 MiB firmware/Brookesia CI artifacts. Do not substitute a CI artifact for a
factory package.
