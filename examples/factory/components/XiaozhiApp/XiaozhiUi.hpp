/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"

namespace esp_brookesia::apps {

/**
 * A portrait conversation page with a voice-reactive orb, connection status,
 * a scrollable transcript and a touch action for the current conversation state.
 *
 * The orb, its glow and the background are drawn once per colour into images; each frame only
 * moves the bars, the eyes and the mouth.
 */
class XiaozhiUi {
public:
    using ActionCallback = void (*)(void *context);

    enum class Mode : uint8_t {
        Offline,
        Busy,           // Preparing or connecting
        Activation,
        Ready,
        Listening,
        Thinking,
        Speaking,
        Error,
    };

    XiaozhiUi() = default;
    ~XiaozhiUi();

    XiaozhiUi(const XiaozhiUi &) = delete;
    XiaozhiUi &operator=(const XiaozhiUi &) = delete;

    bool preload();
    bool create(
        lv_obj_t *parent,
        ActionCallback action_callback,
        void *action_context,
        bool multiline_subtitle = false
    );
    void destroy();

    void setOfflineAction(const char *label);
    void setPaused(bool paused);
    void setStatus(const char *status);
    void setMode(Mode mode);
    /* Loudness of the voice heard or spoken, 0 to 1 */
    void setLevel(float level);
    void setChatMessage(const char *role, const char *text);
    void clearChatMessages();
    /* Shown below the orb while nothing is being said */
    void setHint(const char *hint);
    void setEmotion(const char *emotion);
    void setActivation(const char *code, const char *message, bool visible);

    lv_obj_t *root() const
    {
        return _root;
    }

private:
    enum class Eyes : uint8_t {
        Open,
        Happy,
        Round,
        Sad,
        Squint,
        Wink,
        Closed,
    };

    enum class Mouth : uint8_t {
        None,
        Smile,
        Open,       // Follows the voice while speaking
        Oval,       // Listening
        Flat,
        Frown,
    };

    struct Image {
        lv_image_dsc_t dsc = {};
        uint8_t *pixels = nullptr;
    };

    void destroyView();
    bool allocateImages();
    void releaseImages();
    void renderBackground(uint32_t color);
    void renderOrb(uint32_t color);
    void renderGlow(uint32_t color);
    void applyMode();
    void applyFace();
    void updateAction();
    void updateSubtitle();
    void animate();
    void drawRing(lv_layer_t *layer);
    static Eyes eyesForEmotion(const char *emotion);
    static void tickCallback(lv_timer_t *timer);
    static void clickCallback(lv_event_t *event);
    static void ringDrawCallback(lv_event_t *event);

    lv_obj_t *_root = nullptr;
    lv_obj_t *_background = nullptr;
    lv_obj_t *_glow = nullptr;
    lv_obj_t *_ring = nullptr;
    lv_obj_t *_orb = nullptr;
    lv_obj_t *_eye_pills[2] = {};
    lv_obj_t *_eye_arcs[2] = {};
    lv_obj_t *_mouth = nullptr;
    lv_obj_t *_mouth_arc = nullptr;
    lv_obj_t *_code_label = nullptr;
    lv_obj_t *_chip = nullptr;
    lv_obj_t *_chip_dot = nullptr;
    lv_obj_t *_status_label = nullptr;
    lv_obj_t *_user_label = nullptr;
    lv_obj_t *_reply_box = nullptr;
    lv_obj_t *_reply_label = nullptr;
    lv_obj_t *_action_button = nullptr;
    lv_obj_t *_action_label = nullptr;
    lv_timer_t *_tick = nullptr;

    Image _background_image;
    Image _orb_image;
    Image _glow_image;

    ActionCallback _action_callback = nullptr;
    void *_action_context = nullptr;
    const lv_font_t *_text_font = nullptr;

    Mode _mode = Mode::Offline;
    bool _mode_valid = false;
    Eyes _eyes = Eyes::Open;
    bool _face_valid = false;
    bool _activation_visible = false;
    static constexpr int RING_BARS = 48;

    float _level_target = 0;
    float _level = 0;
    float _phase = 0;
    float _spin = 0;
    float _bars[RING_BARS] = {};
    uint32_t _blink_ms = 0;
    char _hint[96] = {};
    char _offline_action[64] = {};
    char _user_text[512] = {};
    char _reply_text[512] = {};
    char _activation_message[192] = {};
};

} // namespace esp_brookesia::apps
