/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "file_iterator.h"
#include "factory_ui.hpp"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {
class MusicPlayer : public systems::phone::App {
public:
    static MusicPlayer *requestInstance(bool status = true, bool navigation = false);
    ~MusicPlayer();
protected:
    MusicPlayer(bool status, bool navigation);
    bool run() override;
    bool back() override;
    bool close() override;
    bool init() override;
    bool deinit() override;
    bool pause() override;
    bool resume() override;
private:
    static MusicPlayer *_instance;
    file_iterator_instance_t *_file_iterator = nullptr;
    factory_ui::PlayerView _view;
    lv_obj_t *_player = nullptr;
    lv_timer_t *_timer = nullptr;
    size_t _selected = 0;
    bool _player_started = false;
    bool _want_playing = false;
    bool _seen_playing = false;
    bool _last_playing = false;
    uint64_t _elapsed_ms = 0;
    int64_t _last_tick_us = 0;
    bool play_selected();
    void refresh_title();
    static void action_cb(lv_event_t *event);
    static void track_cb(lv_event_t *event);
    static void tick_cb(lv_timer_t *timer);
};
} // namespace esp_brookesia::apps
