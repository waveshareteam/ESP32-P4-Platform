/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Text font with Chinese and English, in 30 px
 *
 *         Glyphs outside the PuHui set, such as LV_SYMBOL_* icons, fall back to the
 *         Montserrat font of the same size. Call from the LVGL task only.
 */
const lv_font_t *app_font_text_30(void);

#ifdef __cplusplus
}
#endif
