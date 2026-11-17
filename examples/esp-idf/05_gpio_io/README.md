# GPIO IO

[中文版本](README_ZH.md)

Small GPIO input/output example.

It toggles one output pin once per second and reads one input pin. If you
connect the output pin to the input pin, the input value should follow the
output value.

## Difficulty

Beginner.

## Hardware Required

- One ESP32-P4 board.
- Optional jumper wire between the configured output and input pins.
- Optional LED with resistor if you want visible output.

## Default Pins

| Signal | Default GPIO |
| --- | --- |
| Output | GPIO2 |
| Input | GPIO3 |

Change these in `idf.py menuconfig` if the pins conflict with your board.

## Build and Flash

```bash
cd examples/esp-idf/05_gpio_io
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

## Expected Output

```text
output=1 input=1
output=0 input=0
```

If no jumper is connected, the input may stay high because the default pull-up
is enabled.

## What to Reuse

- `gpio_config()` for input and output setup.
- `gpio_set_level()` for output control.
- `gpio_get_level()` for input reads.
- `menuconfig` options for board-specific pin selection.
