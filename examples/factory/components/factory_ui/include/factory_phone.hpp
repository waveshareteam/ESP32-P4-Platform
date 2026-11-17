/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace factory_ui {
inline esp_brookesia::systems::phone::App::Config app_config(bool status, bool navigation)
{
    using namespace esp_brookesia::systems::phone;
    auto config = App::Config::SIMPLE_CONSTRUCTOR(nullptr, status, navigation);
    if (navigation) config.navigation_bar_visual_mode = NavigationBar::VisualMode::SHOW_FIXED;
    return config;
}

inline esp_brookesia::systems::base::App::Config screen_config(const char *name, const void *icon)
{
    auto config = esp_brookesia::systems::base::App::Config::SIMPLE_CONSTRUCTOR(name, icon, true);
    // Portrait views reserve the status/navigation regions themselves. Keep
    // their coordinate system at the physical 800 x 1280 display size.
    config.flags.enable_resize_visual_area = 0;
    return config;
}
} // namespace factory_ui
