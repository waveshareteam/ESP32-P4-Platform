/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "factory_ui.hpp"
#include "systems/phone/stylesheets/800_1280/dark/stylesheet.hpp"

inline esp_brookesia::systems::phone::Stylesheet factory_stylesheet()
{
    using namespace esp_brookesia;
    using namespace esp_brookesia::systems::phone;
    auto style = STYLESHEET_800_1280_DARK;
    style.core.name = "Platform 800x1280";
    style.core.screen_size = gui::StyleSize::RECT(factory_ui::WIDTH, factory_ui::HEIGHT);
    style.core.display.background.color = gui::StyleColor::COLOR(factory_ui::BACKGROUND);
    auto &status = style.display.status_bar.data;
    status.main.size = gui::StyleSize::RECT_W_PERCENT(100, factory_ui::STATUS_HEIGHT);
    status.main.background_color = gui::StyleColor::COLOR(factory_ui::BACKGROUND);
    status.main.text_font = gui::StyleFont::SIZE(24);
    status.flags.enable_battery_icon = 0;
    status.flags.enable_battery_label = 0;
    status.icon_common_size = gui::StyleSize::SQUARE(32);
    auto &navigation = style.display.navigation_bar;
    navigation.visual_mode = NavigationBar::VisualMode::SHOW_FIXED;
    navigation.data.main.size = gui::StyleSize::RECT_W_PERCENT(100, factory_ui::NAVIGATION_HEIGHT);
    navigation.data.main.background_color = gui::StyleColor::COLOR(factory_ui::BACKGROUND);
    navigation.data.button.active_background_color = gui::StyleColor::COLOR(factory_ui::ACCENT);
    return style;
}
