/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <cstdint>
#include "ethernet_service.hpp"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

class Ethernet final : public systems::phone::App {
public:
    static Ethernet *requestInstance();

private:
    Ethernet();
    bool run() override;
    bool back() override;
    bool close() override;
    bool pause() override;
    bool resume() override;
    void refresh();

    lv_obj_t *_root = nullptr;
    lv_obj_t *_status = nullptr;
    lv_obj_t *_link = nullptr;
    lv_obj_t *_address = nullptr;
    lv_obj_t *_gateway = nullptr;
    lv_obj_t *_mac = nullptr;
    lv_timer_t *_timer = nullptr;
    uint32_t _revision = UINT32_MAX;
};

} // namespace esp_brookesia::apps
