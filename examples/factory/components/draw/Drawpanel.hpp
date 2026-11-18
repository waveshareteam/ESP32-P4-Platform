/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

/**
 * @brief Drawpanel application for touch-based drawing on the device screen
 *
 */
class Drawpanel: public systems::phone::App {
public:
    /**
     * @brief Get the singleton instance of Drawpanel
     *
     * @param use_status_bar Show status bar
     * @param use_navigation_bar Show navigation bar
     * @return Drawpanel* Singleton instance pointer
     */
    static Drawpanel *requestInstance(bool use_status_bar = false, bool use_navigation_bar = false);

    /**
     * @brief Destroy the Drawpanel object
     *
     */
    ~Drawpanel();

    using systems::phone::App::startRecordResource;
    using systems::phone::App::endRecordResource;

protected:
    /**
     * @brief Construct a new Drawpanel object (private to enforce singleton)
     *
     * @param use_status_bar Show status bar
     * @param use_navigation_bar Show navigation bar
     */
    Drawpanel(bool use_status_bar, bool use_navigation_bar);

    bool run(void) override;
    bool back(void) override;

    bool close(void) override;
    bool init(void) override;
    bool deinit(void) override;
    bool pause(void) override;
    bool resume(void) override;

private:
    static Drawpanel *_instance;

    /* Strokes are drawn into this fixed canvas, which never scrolls */
    lv_obj_t *_canvas = nullptr;
    lv_draw_buf_t *_draw_buf = nullptr;
    lv_obj_t *_selected_swatch = nullptr;
    lv_color_t _pen_color;
    lv_point_t _last_point = {};
    bool _stroke_active = false;

    void createToolbar(void);
    void clearCanvas(void);
    void drawSegment(const lv_point_t &from, const lv_point_t &to);
    static void canvas_event_cb(lv_event_t *e);
    static void swatch_event_cb(lv_event_t *e);
    static void clear_event_cb(lv_event_t *e);
};

} // namespace esp_brookesia::apps
