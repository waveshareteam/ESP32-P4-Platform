/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "factory_ui.hpp"
#include "factory_launcher.hpp"
#include "systems/phone/stylesheets/720_1280/dark/stylesheet.hpp"
#include "systems/phone/stylesheets/800_1280/dark/stylesheet.hpp"

inline esp_brookesia::systems::phone::Stylesheet factory_stylesheet()
{
    using namespace esp_brookesia;
    using namespace esp_brookesia::systems::phone;
    auto style = factory_ui::WIDTH == 720 && factory_ui::HEIGHT == 1280
        ? STYLESHEET_720_1280_DARK : STYLESHEET_800_1280_DARK;
    style.core.name = "Platform Portrait";
    style.core.screen_size = gui::StyleSize::RECT(factory_ui::WIDTH, factory_ui::HEIGHT);
    style.core.display.background.color = gui::StyleColor::COLOR(0x1A1A1A);
    style.display.app_launcher.data = factory_launcher_data();
    auto &status = style.display.status_bar.data;
    status.main.size = gui::StyleSize::RECT_W_PERCENT(100, factory_ui::STATUS_HEIGHT);
    status.main.background_color = gui::StyleColor::COLOR_WITH_OPACITY(0x000000, 0);
    status.main.text_font = gui::StyleFont::SIZE(24);
    status.flags.enable_battery_icon = 0;
    status.flags.enable_battery_label = 0;
#if !CONFIG_ESP_HOSTED_ENABLED || !CONFIG_ESP_WIFI_REMOTE_ENABLED
    status.flags.enable_wifi_icon = 0;
#endif
    status.icon_common_size = gui::StyleSize::SQUARE(32);
    style.display.flags.enable_navigation_bar = 0;
    auto &gesture = style.manager.gesture;
    gesture.threshold.vertical_edge = factory_ui::GESTURE_INSET;
    gesture.threshold.direction_vertical = factory_ui::SWIPE_DISTANCE;
    gesture.threshold.direction_angle = factory_ui::SWIPE_ANGLE;
    gesture.threshold.duration_short_ms = 800;
    auto &bottom = gesture.indicator_bars[static_cast<int>(Gesture::IndicatorBarType::BOTTOM)];
    bottom.main.size_min = gui::StyleSize::RECT(0, 8);
    bottom.main.size_max = gui::StyleSize::RECT_W_PERCENT(28, 8);
    return style;
}
