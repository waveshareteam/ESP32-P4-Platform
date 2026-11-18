/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "factory_phone.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <math.h>
#include <string>
#include <vector>
#include <unistd.h>
#ifdef ESP_UTILS_LOG_TAG
#   undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:App:Calculator"
#include "esp_lib_utils.h"
#include "esp_brookesia_app_calculator.hpp"
#include "factory_ui.hpp"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

LV_IMG_DECLARE(img_app_calculator);

#define KEYBOARD_FONT           &lv_font_montserrat_48
#define LABEL_FONT_SMALL        &lv_font_montserrat_28
#define LABEL_FONT_BIG          &lv_font_montserrat_48

#define APP_NAME                "Calculator"

static constexpr uint32_t DIGIT_COLOR = 0x252A34;
static constexpr uint32_t FUNCTION_COLOR = 0x3A3F4C;
static constexpr uint32_t OPERATOR_COLOR = 0xFF9F0A;
static constexpr uint32_t SECONDARY_TEXT_COLOR = 0x9AA0AC;
static constexpr int KEY_GAP = 14;
static constexpr int KEY_RADIUS = 24;
static constexpr int READOUT_PADDING = 24;
static constexpr int READOUT_GAP = 8;
static constexpr int KEYPAD_ROWS = 5;

struct KeyRow {
    const char *map[5];
    int count;
};
static const KeyRow KEY_ROWS[KEYPAD_ROWS] = {
    {{"C", "/", "x", LV_SYMBOL_BACKSPACE, ""}, 4},
    {{"7", "8", "9", "-", ""}, 4},
    {{"4", "5", "6", "+", ""}, 4},
    {{"1", "2", "3", "%", ""}, 4},
    {{"0", ".", "=", ""}, 3},
};

static uint32_t key_color(int id)
{
    switch (id) {
    case 0: case 3:
        return FUNCTION_COLOR;
    case 1: case 2: case 7: case 11: case 15: case 18:
        return OPERATOR_COLOR;
    default:
        return DIGIT_COLOR;
    }
}

static void fit_readout(lv_obj_t *label)
{
    lv_point_t size;
    lv_text_get_size(&size, lv_label_get_text(label), lv_obj_get_style_text_font(label, LV_PART_MAIN),
                     0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const int width = lv_obj_get_width(lv_obj_get_parent(label));
    const int32_t scale = size.x > width ? std::max<int32_t>(154, width * LV_SCALE_NONE / size.x) : LV_SCALE_NONE;
    lv_obj_set_style_transform_scale(label, scale, 0);
    lv_obj_set_style_transform_pivot_x(label, lv_pct(100), 0);
    lv_obj_set_style_transform_pivot_y(label, lv_pct(50), 0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
}

namespace esp_brookesia::apps {

Calculator *Calculator::_instance = nullptr;

Calculator *Calculator::requestInstance(bool use_status_bar, bool use_navigation_bar)
{
    if (_instance == nullptr) {
        _instance = new Calculator(use_status_bar, use_navigation_bar);
    }
    return _instance;
}

Calculator::Calculator(bool use_status_bar, bool use_navigation_bar) :
   App(factory_ui::screen_config(APP_NAME, &img_app_calculator), factory_ui::app_config(use_status_bar, use_navigation_bar))
{
}

Calculator::~Calculator()
{
    ESP_UTILS_LOGD("Destroy(@0x%p)", this);
}

bool Calculator::init(void)
{
    ESP_UTILS_LOGD("Init(@0x%p)", this);
    return true;
}

bool Calculator::deinit(void)
{
    ESP_UTILS_LOGD("Deinit(@0x%p)", this);
    return true;
}

bool Calculator::run(void)
{
    ESP_UTILS_LOGD("Run(@0x%p)", this);
    _is_starting = true;

    auto visual_area = getVisualArea();
    if (getActiveConfig().status_bar_visual_mode == phone::StatusBar::VisualMode::SHOW_FIXED &&
            visual_area.y1 < factory_ui::STATUS_HEIGHT) {
        visual_area.y1 = factory_ui::STATUS_HEIGHT;
    }
    visual_area.y2 -= factory_ui::GESTURE_INSET;
    const int width = lv_area_get_width(&visual_area) - 2 * factory_ui::GUTTER;
    const int height = lv_area_get_height(&visual_area);
    const int row_gaps = (KEYPAD_ROWS - 1) * KEY_GAP;
    const int key_height = (height * 2 / 3 - row_gaps) / KEYPAD_ROWS;
    const int keypad_height = KEYPAD_ROWS * key_height + row_gaps;
    const int keypad_y = visual_area.y2 + 1 - keypad_height - 16;
    const int content_x = visual_area.x1 + factory_ui::GUTTER;
    const int readout_y = visual_area.y1 + 24;
    const int readout_height = keypad_y - 24 - readout_y;
    const int line_height = lv_font_get_line_height(LABEL_FONT_BIG);
    const int formula_y = readout_height - READOUT_PADDING - 2 * line_height - READOUT_GAP;
    const int history_height = formula_y - READOUT_PADDING - READOUT_GAP;
    if (width <= 2 * READOUT_PADDING || key_height <= 0 || history_height <= 0) {
        _is_starting = false;
        return false;
    }

    formula_len = 1;
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(factory_ui::BACKGROUND), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    int first_id = 0;
    int y = keypad_y;
    for (const auto &keys : KEY_ROWS) {
        auto *row = lv_buttonmatrix_create(screen);
        lv_buttonmatrix_set_map(row, keys.map);
        if (keys.count == 3) {
            lv_buttonmatrix_set_button_width(row, 0, 2);
        }
        lv_obj_set_user_data(row, reinterpret_cast<void *>(static_cast<intptr_t>(first_id)));
        lv_obj_set_size(row, width, key_height);
        lv_obj_set_pos(row, content_x, y);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_column(row, KEY_GAP, 0);
        lv_obj_set_style_radius(row, KEY_RADIUS, LV_PART_ITEMS);
        lv_obj_set_style_border_width(row, 0, LV_PART_ITEMS);
        lv_obj_set_style_shadow_width(row, 0, LV_PART_ITEMS);
        lv_obj_set_style_bg_color(row, lv_color_hex(DIGIT_COLOR), LV_PART_ITEMS);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_ITEMS);
        lv_obj_set_style_text_color(row, lv_color_white(), LV_PART_ITEMS);
        lv_obj_set_style_text_font(row, KEYBOARD_FONT, LV_PART_ITEMS);
        lv_obj_add_flag(row, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
        lv_obj_add_event_cb(row, keyboard_event_cb, LV_EVENT_DRAW_TASK_ADDED, this);
        lv_obj_add_event_cb(row, keyboard_event_cb, LV_EVENT_VALUE_CHANGED, this);
        first_id += keys.count;
        y += key_height + KEY_GAP;
    }

    auto *readout = factory_ui::plain(screen);
    lv_obj_set_size(readout, width, readout_height);
    lv_obj_set_pos(readout, content_x, readout_y);
    lv_obj_set_style_radius(readout, KEY_RADIUS, 0);
    lv_obj_set_style_bg_color(readout, lv_color_hex(factory_ui::CARD), 0);
    lv_obj_set_style_bg_opa(readout, LV_OPA_COVER, 0);

    const int text_width = width - 2 * READOUT_PADDING;
    history_label = lv_textarea_create(readout);
    lv_obj_remove_style_all(history_label);
    lv_obj_set_size(history_label, text_width, history_height);
    lv_obj_set_pos(history_label, READOUT_PADDING, READOUT_PADDING);
    lv_obj_set_style_opa(history_label, LV_OPA_TRANSP, LV_PART_CURSOR);
    lv_obj_set_style_text_color(history_label, lv_color_hex(SECONDARY_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(history_label, LABEL_FONT_SMALL, 0);
    lv_obj_set_style_text_align(history_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_remove_flag(history_label, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_textarea_set_text(history_label, "");

    const auto create_readout = [readout, text_width, line_height](
                                    int y, const char *text, const lv_font_t *font, uint32_t color) {
        auto *container = factory_ui::plain(readout);
        lv_obj_set_size(container, text_width, line_height);
        lv_obj_set_pos(container, READOUT_PADDING, y);
        auto *label = factory_ui::label(container, text, font, color);
        lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
        return label;
    };
    formula_label = create_readout(formula_y, "0", LABEL_FONT_BIG, factory_ui::TEXT);
    result_label = create_readout(formula_y + line_height + READOUT_GAP, "= 0",
                                  LABEL_FONT_SMALL, SECONDARY_TEXT_COLOR);

    _is_starting = false;
    return true;
}

bool Calculator::back(void)
{
    ESP_UTILS_LOGD("Back(@0x%p)", this);

    ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");

    return true;
}

bool Calculator::close(void)
{
    ESP_UTILS_LOGD("Close(@0x%p)", this);
    _is_stopping = true;

    _is_stopping = false;
    return true;
}

bool Calculator::isStartZero(void)
{
    const char *text = lv_label_get_text(formula_label);
    int text_len = strlen(text);

    if ((text_len == 1) && (text[0] == '0')) {
        // LOG_I(TAG, "start 0");
        return true;
    }
    if ((text[text_len - 1] == '0') &&
            (text[text_len - 2] > '9') &&
            (text[text_len - 2] < '0')) {
        // LOG_I(TAG, "start 0");
        return true;
    }

    return false;
}

bool Calculator::isStartNum(void)
{
    const char *text = lv_label_get_text(formula_label);
    int text_len = strlen(text);

    if ((text[text_len - 1] >= '0') && (text[text_len - 1] <= '9')) {
        // LOG_I(TAG, "start num");
        return true;
    }

    return false;
}

bool Calculator::isStartPercent(void)
{
    const char *text = lv_label_get_text(formula_label);
    int text_len = strlen(text);

    if (text[text_len - 1] == '%') {
        // LOG_I(TAG, "start %");
        return true;
    }

    return false;
}

bool Calculator::isLegalDot(void)
{
    const char *text = lv_label_get_text(formula_label);
    int text_len = strlen(text);

    while (text_len-- > 0) {
        if (text[text_len] == '.') {
            // LOG_I(TAG, "illegal dot");
            return false;
        } else if ((text[text_len] < '0') ||
                   (text[text_len] > '9')) {
            return true;
        }
    }

    return true;
}

double Calculator::calculate(const char *input)
{
    std::vector<double> stk;
    int input_len = strlen(input);
    double num = 0;
    bool dot_flag = false;
    int dot_len = 0;
    char pre_sign = '+';

    for (int i = 0; i < input_len; i++) {
        if (input[i] == '.') {
            dot_flag = true;
            dot_len = 0;
        } else if (isdigit(input[i])) {
            if (!dot_flag) {
                num = num * 10 + input[i] - '0';
            } else {
                num += (input[i] - '0') / pow(10.0, ++dot_len);
            }
        } else if (input[i] == '%') {
            num /= 100.0;
        } else if (i != input_len - 1) {
            dot_flag = false;
            dot_len = 0;
            switch (pre_sign) {
            case '+':
                stk.push_back(num);
                break;
            case '-':
                stk.push_back(-num);
                break;
            case 'x':
                stk.back() *= num;
                break;
            default:
                if (num != 0) {
                    stk.back() /= num;
                } else {
                    return 0;
                }
            }
            num = 0;
            pre_sign = input[i];
        }

        if (i == input_len - 1) {
            switch (pre_sign) {
            case '+':
                stk.push_back(num);
                break;
            case '-':
                stk.push_back(-num);
                break;
            case 'x':
                stk.back() *= num;
                break;
            default:
                if (num != 0) {
                    stk.back() /= num;
                } else {
                    return 0;
                }
            }
            num = 0;
        }
    }

    for (int i = 0; i < stk.size(); i++) {
        num += stk.at(i);
    }
    // LOG_I(TAG, "cal: %s = %f", input, num);

    return num;
}

void Calculator::keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    Calculator *app = (Calculator *)lv_event_get_user_data(e);

    auto *row = lv_event_get_target_obj(e);
    const int first_id = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(row)));

    if (code == LV_EVENT_DRAW_TASK_ADDED) {
        auto *task = lv_event_get_draw_task(e);
        auto *descriptor = static_cast<lv_draw_dsc_base_t *>(lv_draw_task_get_draw_dsc(task));
        if (descriptor->part == LV_PART_ITEMS) {
            const uint32_t color = key_color(first_id + descriptor->id1);
            if (auto *fill = lv_draw_task_get_fill_dsc(task)) {
                const bool pressed = lv_buttonmatrix_get_selected_button(row) == descriptor->id1 &&
                                     lv_obj_has_state(row, LV_STATE_PRESSED);
                const lv_color_t base = lv_color_hex(color);
                fill->color = pressed ? lv_color_lighten(base, LV_OPA_30) : base;
            } else if (auto *label = lv_draw_task_get_label_dsc(task); label && color == OPERATOR_COLOR) {
                label->color = lv_color_hex(factory_ui::BACKGROUND);
            }
        }
        return;
    }

    if (code == LV_EVENT_VALUE_CHANGED) {
        const uint32_t selected = lv_buttonmatrix_get_selected_button(row);
        if (selected == LV_BUTTONMATRIX_BUTTON_NONE) {
            return;
        }
        const int btn_id = first_id + selected;
        const char *key = lv_buttonmatrix_get_button_text(row, selected);
        bool calculate_flag = false;
        bool equal_flag = false;
        double res_num;
        char res_str[32];

        if (lv_obj_get_style_text_font(app->formula_label, LV_PART_MAIN) == LABEL_FONT_SMALL) {
            lv_obj_set_style_text_font(app->formula_label, LABEL_FONT_BIG, 0);
            lv_obj_set_style_text_font(app->result_label, LABEL_FONT_SMALL, 0);
        }

        switch (btn_id) {
        // "C"
        case 0:
            lv_label_set_text(app->formula_label, "0");
            app->formula_len = 1;
            calculate_flag = true;
            break;
        // "<"
        case 3:
            if ((app->formula_len == 1) && app->isStartZero()) {
                break;
            }
            lv_label_cut_text(
                app->formula_label,
                --(app->formula_len),
                1
            );
            if (app->formula_len == 0) {
                lv_label_set_text(app->formula_label, "0");
                app->formula_len = 1;
            }
            calculate_flag = true;
            break;
        // "="
        case 18:
            calculate_flag = true;
            equal_flag = true;
            break;
        // "/,X,-,+,%"
        case 1:
        case 2:
        case 7:
        case 11:
        case 15:
            // Need number or percent
            if (app->isStartPercent() ||
                    app->isStartNum()) {
                // Firstly input "+, -"
                if (((btn_id == 7) || (btn_id == 11)) &&
                        (app->isStartZero())) {
                    // Remove "0"
                    lv_label_cut_text(
                        app->formula_label,
                        --(app->formula_len),
                        1
                    );
                }
                lv_label_ins_text(
                    app->formula_label,
                    app->formula_len++,
                    key
                );
                if (btn_id == 15) {
                    calculate_flag = true;
                }
            }
            break;
        // "1234567890"
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 12:
        case 13:
        case 14:
        case 16:
            // Remove First "0"
            if (app->isStartZero()) {
                lv_label_cut_text(
                    app->formula_label,
                    --(app->formula_len),
                    1
                );
            }
            // Except "%"
            if (!app->isStartPercent()) {
                lv_label_ins_text(
                    app->formula_label,
                    app->formula_len++,
                    key
                );
                calculate_flag = true;
            }
            break;
        // "."
        case 17:
            if (app->isLegalDot() && app->isStartNum()) {
                lv_label_ins_text(
                    app->formula_label,
                    app->formula_len++,
                    "."
                );
            }
            break;
        default:
            break;
        }

        if (calculate_flag) {
            lv_obj_set_style_text_font(app->formula_label, LABEL_FONT_BIG, 0);

            res_num = app->calculate(lv_label_get_text(app->formula_label));
            if (int(res_num) == res_num) {
                snprintf(res_str, sizeof(res_str) - 1, "%ld", long(res_num));
            } else {
                // Limit decimal places to fit small screens
                snprintf(res_str, sizeof(res_str) - 1, "%.3f", res_num);
                // Remove trailing zeros
                char *end = res_str + strlen(res_str) - 1;
                while (end > res_str && *end == '0') {
                    *end-- = '\0';
                }
                if (end > res_str && *end == '.') {
                    *end = '\0';
                }
            }
            lv_label_set_text_fmt(app->result_label, "= %s", res_str);
            lv_obj_set_style_text_font(app->result_label, LABEL_FONT_SMALL, 0);
        }

        if (equal_flag) {
            lv_obj_set_style_text_font(app->result_label, LABEL_FONT_BIG, 0);

            std::string entry = *lv_textarea_get_text(app->history_label) ? "\n" : "";
            entry += lv_label_get_text(app->formula_label);
            entry += " = ";
            entry += res_str;
            lv_textarea_set_cursor_pos(app->history_label, LV_TEXTAREA_CURSOR_LAST);
            lv_textarea_add_text(app->history_label, entry.c_str());

            lv_label_set_text_fmt(app->formula_label, "%s", res_str);
            lv_obj_set_style_text_font(app->formula_label, LABEL_FONT_SMALL, 0);
            app->formula_len = strlen(res_str);
        }
        lv_obj_set_style_text_color(app->formula_label,
                                    lv_color_hex(equal_flag ? SECONDARY_TEXT_COLOR : 0xFFFFFF), 0);
        lv_obj_set_style_text_color(app->result_label,
                                    lv_color_hex(equal_flag ? 0xFFFFFF : SECONDARY_TEXT_COLOR), 0);
        lv_obj_update_layout(lv_obj_get_parent(app->formula_label));
        fit_readout(app->formula_label);
        fit_readout(app->result_label);
    }
}

}
