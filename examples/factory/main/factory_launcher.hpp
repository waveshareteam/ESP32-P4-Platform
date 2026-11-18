/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "factory_ui.hpp"
#include "systems/phone/stylesheets/800_1280/dark/app_launcher_data.hpp"
#include "systems/phone/assets/esp_brookesia_phone_assets.h"

inline esp_brookesia::systems::phone::AppLauncherData factory_launcher_data()
{
    using namespace esp_brookesia;
    auto data = systems::phone::STYLESHEET_800_1280_DARK_APP_LAUNCHER_DATA;
    // Keep the reference launcher's four icons per page, with portrait spacing.
    data.table.default_num = 1;
    data.table.size = gui::StyleSize::RECT_W_PERCENT(100, 1120);
    data.icon.main.size = gui::StyleSize::RECT(320, 480);
    data.icon.main.layout_row_pad = 20;
    data.icon.image.default_size = gui::StyleSize::SQUARE(192);
    data.icon.image.press_size = gui::StyleSize::SQUARE(172);
    data.icon.label.text_font = gui::StyleFont::SIZE(28);
    data.indicator.main_size = gui::StyleSize::RECT_W_PERCENT(100, 64);
    data.indicator.main_layout_bottom_offset = 16;
    data.indicator.spot_inactive_size = gui::StyleSize::SQUARE(12);
    data.indicator.spot_active_size = gui::StyleSize::RECT(30, 12);
    return data;
}

inline lv_obj_t *factory_launcher_wallpaper(lv_obj_t *screen)
{
    auto *wallpaper = lv_image_create(screen);
    lv_image_set_src(wallpaper, &esp_brookesia_image_middle_wallpaper_dark_480_480);
    lv_obj_set_size(wallpaper, factory_ui::WIDTH, factory_ui::HEIGHT);
    lv_image_set_inner_align(wallpaper, LV_IMAGE_ALIGN_CONTAIN);
    lv_obj_center(wallpaper);
    lv_obj_remove_flag(wallpaper, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_to_index(wallpaper, 0);
    return wallpaper;
}
