/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <cstddef>
#include <cstring>

namespace esp_brookesia::apps {

inline void copyUtf8(char *destination, size_t capacity, const char *source)
{
    if (!destination || capacity == 0) {
        return;
    }
    source = source ? source : "";
    size_t length = 0;
    while (length < capacity - 1 && source[length] != '\0') {
        ++length;
    }
    // If the next byte continues the last character, drop that partial character.
    while (length > 0 && (static_cast<unsigned char>(source[length]) & 0xC0) == 0x80) {
        --length;
    }
    std::memcpy(destination, source, length);
    destination[length] = '\0';
}

} // namespace esp_brookesia::apps
