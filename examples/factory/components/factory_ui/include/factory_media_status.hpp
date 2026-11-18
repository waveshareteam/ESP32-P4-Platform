/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "factory_ui.hpp"

namespace factory_ui {
// The Knob player places the icon, title and help in a centered vertical stack.
inline void show_media_status(lv_obj_t *&view, const char *symbol, const char *title, const char *hint = nullptr)
{
    if (!view) {
        view = plain(lv_screen_active());
        lv_obj_set_size(view, WIDTH - 2 * GUTTER, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(view, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(view, 20, 0);
        lv_obj_center(view);
        label(view, "", &lv_font_montserrat_48, 0xA066FF);
        auto *heading = label(view, "", &lv_font_montserrat_32);
        auto *description = label(view, "", &lv_font_montserrat_24, 0xA0A3B1);
        lv_obj_t *text_labels[] = {heading, description};
        for (auto *text : text_labels) {
            lv_obj_set_width(text, lv_pct(100));
            lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);
            lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
        }
    }
    lv_label_set_text(lv_obj_get_child(view, 0), symbol);
    lv_label_set_text(lv_obj_get_child(view, 1), title);
    auto *description = lv_obj_get_child(view, 2);
    if (hint && *hint) {
        lv_label_set_text(description, hint);
        lv_obj_remove_flag(description, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(description, LV_OBJ_FLAG_HIDDEN);
    }
}
} // namespace factory_ui
