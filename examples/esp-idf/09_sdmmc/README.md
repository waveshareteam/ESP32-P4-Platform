# SDMMC

[中文版本](README_ZH.md)

Mount and test an SD card over the SDMMC interface.

## Difficulty

Intermediate.

## Hardware Required

- ESP32-P4 board with SD card slot or wired SD card module.
- SD card.

## Build and Flash

```bash
cd examples/esp-idf/09_sdmmc
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

In **SD/MMC Example Configuration**, check:

- Bus width: 4-line or 1-line mode.
- CMD, CLK, D0, D1, D2, D3 GPIOs.
- Format options.
- SD power control options.

Do not enable format options unless you are willing to erase the card.

Default ESP32-P4 SDMMC pins:

| Signal | Default GPIO |
| --- | --- |
| CLK | GPIO43 |
| CMD | GPIO44 |
| D0 | GPIO39 |
| D1 | GPIO40 |
| D2 | GPIO41 |
| D3 | GPIO42 |

The default bus width is 4-line mode. If you are using hand wiring or a simple
module, switch to 1-line mode first and only enable 4-line mode after the card
mounts reliably.

## Expected Output

The example mounts the card, prints card information, writes
`/sdcard/hello.txt`, renames it to `/sdcard/foo.txt`, reads it back, writes
`/sdcard/nihao.txt`, reads it back, and unmounts the card.

## Troubleshooting

- Try 1-line mode when wiring is uncertain.
- Check card detect and power wiring if your board uses them.
- Use a known-good SD card formatted as FAT.
- Keep SDMMC wires short when using an external module.
- The example enables internal pull-ups for convenience, but SD cards still
  need proper external pull-ups on the bus in real hardware.
- On ESP32-P4 boards using an internal LDO for SD power, verify the configured
  LDO ID against the schematic before enabling that option.
