/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "cbin_font.h"
#include "app_fonts.h"

extern const uint8_t font_30_start[] asm("_binary_font_puhui_common_30_4_bin_start");

static const char *TAG = "app_fonts";

static const lv_font_t *load(lv_font_t **cache, const uint8_t *bin, const lv_font_t *fallback)
{
    if (!*cache) {
        // The font tables stay in flash; only the small descriptors are copied to RAM
        *cache = cbin_font_create((uint8_t *)bin);
        if (!*cache) {
            ESP_LOGE(TAG, "Load font failed");
            return fallback;
        }
        (*cache)->fallback = fallback;
    }
    return *cache;
}

const lv_font_t *app_font_text_30(void)
{
    static lv_font_t *font;
    return load(&font, font_30_start, &lv_font_montserrat_30);
}
