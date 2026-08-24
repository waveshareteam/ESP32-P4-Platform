# Changelog

[简体中文](CHANGELOG_CN.md)

All notable repository-level changes should be documented in this file.

This project does not currently publish versioned releases. Until releases are
introduced, use dated entries for user-facing changes.

## Unreleased

- Make the ESP32-P4 v3.x profile the default for ESP-IDF examples and Arduino
  CI while retaining explicit pre-v3/v1.3 build profiles.
- Probe GT911 at `0x5D` and then `0x14` in Arduino polling examples without
  assigning INT or RST GPIOs, and continue safely when touch is unavailable.
- Add Arduino camera preview, camera ISP tuning, SD-card, and ES8311 audio
  playback examples with bilingual build and HIL guidance.
- Add open-source project documentation, contribution guidance, security
  policy, issue templates, pull request template, and example index.
- Add beginner-oriented getting started, troubleshooting, examples guide, and
  example roadmap documents.
- Add the `00_board_check` ESP-IDF example as a no-extra-hardware first-run
  validation project.
- Add `03_nvs_counter`, `04_freertos_tasks`, `05_gpio_io`,
  `06_gpio_interrupt`, `07_uart_loopback`, and `19_app_skeleton` as
  beginner-to-advanced ESP-IDF examples.
- Reorder ESP-IDF examples into a 20-demo learning path from basic validation
  to integrated application structure.
