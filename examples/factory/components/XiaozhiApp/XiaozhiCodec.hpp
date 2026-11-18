/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "product_support.h"

namespace esp_brookesia::apps {

inline esp_err_t prepareXiaozhiCodec()
{
    // Other factory media can leave the codec muted or at a different sample rate.
    const esp_err_t format = product_codec_dev_resume();
    if (format != ESP_OK) {
        return format;
    }
    const esp_err_t unmute = product_codec_mute_set(false);
    if (unmute != ESP_OK) {
        return unmute;
    }
    return product_codec_volume_set(product_codec_volume_get(), nullptr);
}

} // namespace esp_brookesia::apps
