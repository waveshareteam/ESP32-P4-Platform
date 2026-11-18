/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "esp_err.h"

namespace esp_brookesia::apps {

class XiaozhiActivationClient final {
public:
    enum class BindingState : uint8_t {
        Bound,
        ActivationRequired,
    };

    struct Info {
        BindingState state = BindingState::Bound;
        int http_status = 0;
        int timeout_ms = 0;
        bool factory_identity = false;
        bool has_websocket_config = false;
        int websocket_version = 1;
        std::string websocket_url;
        std::string websocket_token;
        std::string device_id;
        std::string client_id;
        std::string code;
        std::string message;
        std::string challenge;
    };

    XiaozhiActivationClient() = default;

    XiaozhiActivationClient(const XiaozhiActivationClient &) = delete;
    XiaozhiActivationClient &operator=(const XiaozhiActivationClient &) = delete;

    esp_err_t fetchInfo(Info &out);

    // ESP_OK means bound, ESP_ERR_TIMEOUT means the web binding is pending.
    esp_err_t activate(const Info &info, int &http_status);

private:
    struct Identity {
        char uuid[37] = {};
        char mac_address[18] = {};
        char serial_number[33] = {};
        bool factory = false;
    };

    esp_err_t loadIdentity(Identity &identity) const;
    esp_err_t buildBoardJson(const Identity &identity, std::string &json) const;
    esp_err_t performPost(
        const char *url,
        const Identity &identity,
        const char *body,
        std::string *response,
        int &http_status
    );
};

} // namespace esp_brookesia::apps
