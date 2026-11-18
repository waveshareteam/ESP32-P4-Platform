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
    return App::Config::SIMPLE_CONSTRUCTOR(nullptr, status, navigation);
}

inline esp_brookesia::systems::base::App::Config screen_config(const char *name, const void *icon)
{
    auto config = esp_brookesia::systems::base::App::Config::SIMPLE_CONSTRUCTOR(name, icon, true);
    // Portrait views reserve the status/navigation regions themselves. Keep
    // their coordinate system at the physical display size.
    config.flags.enable_resize_visual_area = 0;
    return config;
}
} // namespace factory_ui
