/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <cstdint>

namespace factory_ethernet {

enum class State { Starting, NoLink, ObtainingAddress, Connected, Error };

struct Snapshot {
    State state = State::Starting;
    uint32_t revision = 0;
    char link[40] = {};
    char address[16] = {};
    char gateway[16] = {};
    char mac[18] = {};
};

Snapshot snapshot();

// The factory initializes esp-netif and the default event loop before calling this.
bool start();

} // namespace factory_ethernet
