/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "factory_phone.hpp"
#include "MusicPlayer.hpp"
#include "product_support.h"
#include "bsp/esp-bsp.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string>

LV_IMG_DECLARE(img_app_musicplayer);
namespace esp_brookesia::apps {
MusicPlayer *MusicPlayer::_instance = nullptr;
MusicPlayer *MusicPlayer::requestInstance(bool status, bool navigation)
{
    if (!_instance) _instance = new MusicPlayer(status, navigation);
    return _instance;
}
MusicPlayer::MusicPlayer(bool status, bool navigation) : App(factory_ui::screen_config("MusicPlayer", &img_app_musicplayer), factory_ui::app_config(status, navigation)) {}
MusicPlayer::~MusicPlayer() {}

static std::string track_name(file_iterator_instance_t *iterator, size_t index)
{
    const char *file = file_iterator_get_name_from_index(iterator, index);
    std::string name = file ? file : "Music";
    auto extension = name.find_last_of('.');
    if (extension != std::string::npos) name.resize(extension);
    return name;
}

bool MusicPlayer::run()
{
    if (product_player_init() != ESP_OK) return false;
    _player_started = true;
    if (product_file_instance_init(BSP_SPIFFS_MOUNT_POINT "/music", &_file_iterator) != ESP_OK ||
        file_iterator_get_count(_file_iterator) == 0) {
        close();
        return false;
    }
    _selected = 0;
    _elapsed_ms = 0;
    _want_playing = _seen_playing = _last_playing = false;
    _player = _view.create(lv_screen_active(), action_cb, this);
    const size_t count = file_iterator_get_count(_file_iterator);
    for (size_t i = 0; i < count; ++i) {
        _view.add_track(track_name(_file_iterator, i).c_str(), track_cb, this);
    }
    refresh_title();
    _last_tick_us = esp_timer_get_time();
    _timer = lv_timer_create(tick_cb, 250, this);
    return _timer != nullptr;
}

void MusicPlayer::refresh_title()
{
    _view.title(track_name(_file_iterator, _selected).c_str(), static_cast<int>(_selected),
                static_cast<int>(file_iterator_get_count(_file_iterator)));
}

bool MusicPlayer::play_selected()
{
    if (!_player_started) {
        if (product_player_init() != ESP_OK) return false;
        _player_started = true;
    }
    file_iterator_set_index(_file_iterator, _selected);
    if (product_player_play_index(_file_iterator, static_cast<int>(_selected)) != ESP_OK) {
        _view.message("Unable to play this file");
        return false;
    }
    _elapsed_ms = 0;
    _last_tick_us = esp_timer_get_time();
    _want_playing = true;
    _seen_playing = _last_playing = false;
    refresh_title();
    _view.elapsed(0);
    return true;
}

void MusicPlayer::action_cb(lv_event_t *event)
{
    auto *self = static_cast<MusicPlayer *>(lv_event_get_user_data(event));
    auto *button = static_cast<lv_obj_t *>(lv_event_get_current_target(event));
    auto action = static_cast<factory_ui::PlayerAction>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(button)));
    const size_t count = file_iterator_get_count(self->_file_iterator);
    switch (action) {
    case factory_ui::PlayerAction::PREVIOUS:
        self->_selected = (self->_selected + count - 1) % count;
        self->play_selected();
        break;
    case factory_ui::PlayerAction::NEXT:
        self->_selected = (self->_selected + 1) % count;
        self->play_selected();
        break;
    case factory_ui::PlayerAction::PLAY_PAUSE:
        if (self->_player_started && audio_player_get_state() == AUDIO_PLAYER_STATE_PLAYING) {
            self->_want_playing = false;
            audio_player_pause();
        } else if (self->_player_started && audio_player_get_state() == AUDIO_PLAYER_STATE_PAUSE) {
            self->_want_playing = true;
            audio_player_resume();
        } else {
            self->play_selected();
        }
        break;
    }
}

void MusicPlayer::track_cb(lv_event_t *event)
{
    auto *self = static_cast<MusicPlayer *>(lv_event_get_user_data(event));
    auto *track = static_cast<lv_obj_t *>(lv_event_get_current_target(event));
    self->_selected = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(track));
    self->play_selected();
}

void MusicPlayer::tick_cb(lv_timer_t *timer)
{
    auto *self = static_cast<MusicPlayer *>(lv_timer_get_user_data(timer));
    const int64_t now = esp_timer_get_time();
    if (!self->_player_started) {
        self->_last_tick_us = now;
        return;
    }
    const auto state = audio_player_get_state();
    const bool playing = state == AUDIO_PLAYER_STATE_PLAYING;
    if (playing && self->_last_playing) self->_elapsed_ms += (now - self->_last_tick_us) / 1000;
    self->_last_tick_us = now;
    self->_last_playing = playing;
    self->_view.playing(playing);
    self->_view.elapsed(static_cast<uint32_t>(self->_elapsed_ms / 1000));
    if (playing) self->_seen_playing = true;
    if (state == AUDIO_PLAYER_STATE_IDLE && self->_seen_playing && self->_want_playing) {
        self->_selected = (self->_selected + 1) % file_iterator_get_count(self->_file_iterator);
        self->play_selected();
    } else if (state == AUDIO_PLAYER_STATE_IDLE && !self->_want_playing) {
        self->_view.message("Ready to play");
    }
}

bool MusicPlayer::back()
{
    return notifyCoreClosed();
}
bool MusicPlayer::close()
{
    if (_timer) lv_timer_delete(_timer);
    _timer = nullptr;
    _want_playing = false;
    if (_player_started) product_player_del();
    _player_started = false;
    product_file_instance_del(_file_iterator);
    _file_iterator = nullptr;
    if (_player) lv_obj_delete(_player);
    _player = nullptr;
    _view.clear();
    return true;
}
bool MusicPlayer::init() { return true; }
bool MusicPlayer::deinit() { return true; }
bool MusicPlayer::pause()
{
    if (_timer) lv_timer_pause(_timer);
    // Other media apps share this codec. Release the decoder while this view is
    // in the background; the selected track is started on demand when revisited.
    if (_player_started) product_player_del();
    _player_started = _want_playing = _seen_playing = _last_playing = false;
    return true;
}
bool MusicPlayer::resume()
{
    _elapsed_ms = 0;
    _last_tick_us = esp_timer_get_time();
    _view.playing(false);
    _view.message("Ready to play");
    _view.elapsed(0);
    if (_timer) lv_timer_resume(_timer);
    return true;
}
} // namespace esp_brookesia::apps
