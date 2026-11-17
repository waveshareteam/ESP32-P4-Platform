# I2C Tools

[中文版本](README_ZH.md)

Interactive I2C command-line tools for scanning and debugging devices on an
I2C bus.

## Difficulty

Beginner to intermediate.

## Hardware Required

- One ESP32-P4 board.
- Optional I2C device connected to the configured SDA/SCL pins.
- Pull-up resistors if the board or module does not already provide them.

The example can still run without an external I2C device, but scans will not
find anything.

## Build and Flash

```bash
cd examples/esp-idf/08_i2c_tools
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Configuration

In `menuconfig`, check **Example Configuration**:

- `SCL GPIO Num`
- `SDA GPIO Num`
- `Store command history in flash`

Default ESP32-P4 pins:

| Signal | Default GPIO |
| --- | --- |
| SDA | GPIO7 |
| SCL | GPIO8 |

The initial bus setup does not enable internal pull-ups, so external pull-ups
or module-provided pull-ups are recommended for reliable scans. The
`i2cconfig` command can reconfigure the bus at runtime and enables internal
pull-ups for that runtime bus instance.

When command history is enabled, the example uses the custom FAT partition from
`partitions_example.csv` to save console history in flash.

## Common Commands

After the monitor opens, use the prompt:

```text
i2c-tools> i2cdetect
i2c-tools> i2cconfig --sda 7 --scl 8 --freq 400000
i2c-tools> i2cget -c 0x50 -r 0x00 -l 1
i2c-tools> i2cset -c 0x50 -r 0x00 0x12
```

Use `help` in the console to list supported commands and arguments.

## Troubleshooting

- If every address is empty, check SDA/SCL pins, power, ground, and pull-ups.
- If many addresses appear at once, check for wiring problems or missing
  pull-ups.
- If the target device is 5 V, confirm the I2C pins are level-shifted.
- If `i2cget` or `i2cset` fails after a scan succeeds, check whether the device
  expects 8-bit or 16-bit register addressing before reusing the command in an
  application.
