/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <cstdint>

namespace factory_wifi {

enum class State { Off, Starting, Scanning, Idle, Connecting, Connected, Error };

struct Network {
    char ssid[33];
    int rssi;
    bool secured;
};

inline constexpr int MAX_NETWORKS = 12;

struct Snapshot {
    State state;
    bool enabled;
    char ssid[33];
    char ip[16];
    char message[96];
    int count;
    Network networks[MAX_NETWORKS];
    uint32_t revision;
};

bool start();
Snapshot snapshot();
bool set_enabled(bool enabled);
bool scan();
bool connect(const Network &network, const char *password);
bool forget();

} // namespace factory_wifi
