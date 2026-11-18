/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Ethernet.hpp"

#include <array>
#include <cstring>
#include "factory_phone.hpp"
#include "factory_ui.hpp"

namespace esp_brookesia::apps {
namespace {

// Rasterize a rounded launcher tile with an RJ45 jack and its contacts.
constexpr int ICON_SIZE = 64;
constexpr auto icon_pixels = [] {
    std::array<uint32_t, ICON_SIZE * ICON_SIZE> pixels{};
    for (int y = 0; y < ICON_SIZE; ++y) {
        for (int x = 0; x < ICON_SIZE; ++x) {
            const int rx = x < 12 ? 12 - x : x > 51 ? x - 51 : 0;
            const int ry = y < 12 ? 12 - y : y > 51 ? y - 51 : 0;
            if (rx * rx + ry * ry > 12 * 12) continue;

            uint32_t color = 0xff123968;
            const bool jack = (x >= 14 && x < 50 && y >= 15 && y < 39) ||
                              (x >= 23 && x < 41 && y >= 39 && y < 49);
            const bool opening = (x >= 18 && x < 46 && y >= 19 && y < 35) ||
                                 (x >= 27 && x < 37 && y >= 35 && y < 45);
            if (jack && !opening) color = 0xffe6f4ff;
            if (x >= 20 && x < 44 && y >= 19 && y < 26 && (x - 20) % 6 < 3) color = 0xff3d9bff;
            pixels[y * ICON_SIZE + x] = color;
        }
    }
    return pixels;
}();

const lv_image_dsc_t icon = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_ARGB8888,
        .w = ICON_SIZE,
        .h = ICON_SIZE,
        .stride = ICON_SIZE * sizeof(uint32_t),
    },
    .data_size = sizeof(icon_pixels),
    .data = reinterpret_cast<const uint8_t *>(icon_pixels.data()),
};

const char *status_label(factory_ethernet::State state)
{
    switch (state) {
    case factory_ethernet::State::Starting: return "Starting Ethernet";
    case factory_ethernet::State::NoLink: return "Plug in an Ethernet cable";
    case factory_ethernet::State::ObtainingAddress: return "Waiting for DHCP";
    case factory_ethernet::State::Connected: return "Connected";
    case factory_ethernet::State::Error: return "Unable to start Ethernet";
    }
    return "";
}

void set_value(lv_obj_t *label, const char *value)
{
    const char *text = *value ? value : "-";
    if (std::strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

} // namespace

Ethernet *Ethernet::requestInstance()
{
    static Ethernet app;
    return &app;
}

Ethernet::Ethernet() :
    App(factory_ui::screen_config("Ethernet", &icon), factory_ui::app_config(true, false))
{
}

bool Ethernet::run()
{
    const auto page = factory_ui::page(lv_screen_active(), "Ethernet", nullptr, nullptr);
    _root = page.root;
    _status = factory_ui::row(page.content, &icon, "Connection", "").value;
    _link = factory_ui::row(page.content, nullptr, "Link speed", "").value;
    _address = factory_ui::row(page.content, nullptr, "IPv4 address", "").value;
    _gateway = factory_ui::row(page.content, nullptr, "Gateway", "").value;
    _mac = factory_ui::row(page.content, nullptr, "MAC address", "").value;
    _revision = UINT32_MAX;
    refresh();

    _timer = lv_timer_create([](lv_timer_t *timer) {
        static_cast<Ethernet *>(lv_timer_get_user_data(timer))->refresh();
    }, 500, this);
    if (!_timer) {
        close();
        return false;
    }
    return true;
}

bool Ethernet::back()
{
    notifyCoreClosed();
    return true;
}

bool Ethernet::close()
{
    if (_timer) {
        lv_timer_delete(_timer);
        _timer = nullptr;
    }
    if (_root) {
        lv_obj_delete(_root);
        _root = nullptr;
    }
    return true;
}

bool Ethernet::pause()
{
    if (_timer) lv_timer_pause(_timer);
    return true;
}

bool Ethernet::resume()
{
    if (_timer) {
        refresh();
        lv_timer_resume(_timer);
    }
    return true;
}

void Ethernet::refresh()
{
    const auto current = factory_ethernet::snapshot();
    if (current.revision == _revision) return;
    _revision = current.revision;
    set_value(_status, status_label(current.state));
    set_value(_link, current.link);
    set_value(_address, current.address);
    set_value(_gateway, current.gateway);
    set_value(_mac, current.mac);
}

} // namespace esp_brookesia::apps
