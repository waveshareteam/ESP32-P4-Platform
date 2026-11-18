/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "wifi_service.hpp"

#include <atomic>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"

namespace factory_wifi {
namespace {

constexpr char TAG[] = "factory_wifi";
constexpr char NVS_NAMESPACE[] = "storage";
constexpr char NVS_ENABLE_KEY[] = "wifi_en";
constexpr TickType_t CONNECT_TIMEOUT = pdMS_TO_TICKS(20000);

enum class Action { Enable, Disable, Scan, Connect, Forget, GotIP, Disconnected };

struct Command {
    Action action;
    Network network;
    char password[65];
    esp_ip4_addr_t ip;
    uint8_t reason;
};

std::atomic<QueueHandle_t> commands{};
std::atomic_flag starting = ATOMIC_FLAG_INIT;
portMUX_TYPE snapshot_lock = portMUX_INITIALIZER_UNLOCKED;
Snapshot published{};

bool post(QueueHandle_t queue, const Command &command)
{
    return queue && xQueueSend(queue, &command, 0) == pdPASS;
}

bool post(Action action)
{
    return post(commands.load(), Command{action});
}

void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    auto queue = static_cast<QueueHandle_t>(arg);
    if (!data) {
        return;
    }

    Command command{};
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        command.action = Action::GotIP;
        command.ip = static_cast<const ip_event_got_ip_t *>(data)->ip_info.ip;
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        const auto &event = *static_cast<const wifi_event_sta_disconnected_t *>(data);
        command.action = Action::Disconnected;
        memcpy(command.network.ssid, event.ssid, sizeof(event.ssid));
        command.reason = event.reason;
    } else {
        return;
    }

    if (!post(queue, command)) {
        ESP_LOGW(TAG, "Wi-Fi command queue is full, event %" PRId32 " was skipped", id);
    }
}

bool read_enabled()
{
    nvs_handle_t nvs;
    int32_t enabled = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_i32(nvs, NVS_ENABLE_KEY, &enabled);
        nvs_close(nvs);
    }
    return enabled != 0;
}

void save_enabled(bool enabled)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        err = nvs_set_i32(nvs, NVS_ENABLE_KEY, enabled);
        if (err == ESP_OK) {
            err = nvs_commit(nvs);
        }
        nvs_close(nvs);
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Save Wi-Fi setting: %s", esp_err_to_name(err));
    }
}

// Only this task owns the radio and the mutable connection state. The GUI reads
// snapshots and the ESP event loop only queues events.
class Worker {
public:
    explicit Worker(QueueHandle_t queue) : queue(queue) {}

    void run()
    {
        if (read_enabled()) {
            enable(false);
        }

        for (;;) {
            TickType_t wait = portMAX_DELAY;
            if (current.state == State::Connecting) {
                const TickType_t elapsed = xTaskGetTickCount() - connect_started;
                wait = elapsed < CONNECT_TIMEOUT ? CONNECT_TIMEOUT - elapsed : 0;
            }

            Command command{};
            if (xQueueReceive(queue, &command, wait) == pdPASS) {
                switch (command.action) {
                case Action::Enable: enable(); break;
                case Action::Disable: disable(); break;
                case Action::Scan:
                    if (current.enabled && current.state != State::Connecting && start_radio()) {
                        scan_networks();
                    }
                    break;
                case Action::Connect: join(command); break;
                case Action::Forget: forget_network(); break;
                case Action::GotIP: got_ip(command.ip); break;
                case Action::Disconnected: disconnected(command); break;
                }
                explicit_bzero(command.password, sizeof(command.password));
            } else if (current.state == State::Connecting) {
                clear_connection();
                status(State::Error, "Connection timed out. Try joining the network again.");
                esp_wifi_disconnect();
            }
        }
    }

private:
    QueueHandle_t queue;
    Snapshot current{};
    esp_netif_t *station = nullptr;
    esp_event_handler_instance_t disconnect_handler = nullptr;
    esp_event_handler_instance_t ip_handler = nullptr;
    bool initialized = false;
    bool radio_started = false;
    bool joining_after_disconnect = false;
    unsigned reconnects_left = 0;
    TickType_t connect_started = 0;

    void status(State state, const char *message = "")
    {
        current.state = state;
        strlcpy(current.message, message, sizeof(current.message));
        ++current.revision;
        portENTER_CRITICAL(&snapshot_lock);
        published = current;
        portEXIT_CRITICAL(&snapshot_lock);
    }

    void error(const char *operation, esp_err_t err, const char *message)
    {
        ESP_LOGW(TAG, "%s: %s (0x%x)", operation, esp_err_to_name(err), static_cast<unsigned>(err));
        status(State::Error, message);
    }

    esp_err_t initialize()
    {
        if (!station) {
            esp_err_t err = esp_netif_init();
            if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
                return err;
            }
            err = esp_event_loop_create_default();
            if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
                return err;
            }
            station = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (!station) {
                station = esp_netif_create_default_wifi_sta();
                if (!station) {
                    return ESP_ERR_NO_MEM;
                }
            }
        }

        if (!disconnect_handler) {
            const esp_err_t err = esp_event_handler_instance_register(
                WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event, queue, &disconnect_handler);
            if (err != ESP_OK) {
                return err;
            }
        }
        if (!ip_handler) {
            const esp_err_t err = esp_event_handler_instance_register(
                IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, queue, &ip_handler);
            if (err != ESP_OK) {
                return err;
            }
        }
        if (!initialized) {
            const wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
            const esp_err_t err = esp_wifi_init(&config);
            if (err != ESP_OK) {
                return err;
            }
            initialized = true;
        }
        return ESP_OK;
    }

    bool start_radio()
    {
        if (radio_started) {
            return true;
        }

        status(State::Starting, "Starting Wi-Fi…");
        esp_err_t err = initialize();
        if (err == ESP_OK) {
            err = esp_wifi_set_mode(WIFI_MODE_STA);
        }
        if (err == ESP_OK) {
            err = esp_wifi_start();
        }
        if (err != ESP_OK) {
            error("Start Wi-Fi", err, "Could not start Wi-Fi. Try turning it off and on.");
            return false;
        }
        radio_started = true;
        return true;
    }

    void enable(bool persist = true)
    {
        const bool was_enabled = current.enabled;
        current.enabled = true;
        if (!was_enabled && persist) {
            save_enabled(true);
        }
        if (current.state == State::Connecting || !start_radio()) {
            return;
        }
        scan_networks();

        if (!current.ip[0]) {
            wifi_config_t config{};
            const esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &config);
            if (err == ESP_OK && config.sta.ssid[0]) {
                memcpy(current.ssid, config.sta.ssid, sizeof(config.sta.ssid));
                current.ssid[sizeof(config.sta.ssid)] = '\0';
                try_connect();
            }
            explicit_bzero(&config, sizeof(config));
        }
    }

    void clear_connection()
    {
        current.ssid[0] = '\0';
        current.ip[0] = '\0';
        reconnects_left = 0;
        joining_after_disconnect = false;
    }

    void disable()
    {
        current.enabled = false;
        current.count = 0;
        clear_connection();
        status(State::Off);
        save_enabled(false);
        if (radio_started) {
            const esp_err_t err = esp_wifi_stop();
            if (err == ESP_OK || err == ESP_ERR_WIFI_NOT_STARTED) {
                radio_started = false;
            } else {
                error("Stop Wi-Fi", err, "Could not turn Wi-Fi off. Try again.");
            }
        }
    }

    void scan_networks()
    {
        status(State::Scanning, "Looking for networks…");
        wifi_ap_record_t records[MAX_NETWORKS]{};
        uint16_t count = 0;
        esp_err_t err = esp_wifi_scan_start(nullptr, true);
        if (err == ESP_OK) {
            err = esp_wifi_scan_get_ap_num(&count);
        }
        if (err == ESP_OK && count > 0) {
            if (count > MAX_NETWORKS) {
                count = MAX_NETWORKS;
            }
            err = esp_wifi_scan_get_ap_records(&count, records);
        }
        if (err != ESP_OK) {
            error("Scan for networks", err, "Could not scan for networks. Try again.");
            return;
        }

        current.count = 0;
        for (uint16_t i = 0; i < count; ++i) {
            const auto &record = records[i];
            Network network{};
            memcpy(network.ssid, record.ssid, sizeof(network.ssid) - 1);
            if (!network.ssid[0]) {
                continue;
            }
            network.rssi = record.rssi;
            network.secured = record.authmode != WIFI_AUTH_OPEN;
            bool duplicate = false;
            for (int j = 0; j < current.count; ++j) {
                const auto &existing = current.networks[j];
                duplicate = network.secured == existing.secured && strcmp(network.ssid, existing.ssid) == 0;
                if (duplicate) {
                    break;
                }
            }
            if (!duplicate) {
                current.networks[current.count++] = network;
            }
        }
        ESP_LOGI(TAG, "Found %d Wi-Fi networks", current.count);
        if (current.ip[0]) {
            status(State::Connected);
        } else {
            status(State::Idle, current.count ? "" : "No networks found. Try scanning again.");
        }
    }

    void join(const Command &command)
    {
        if (!current.enabled || !start_radio()) {
            return;
        }
        const bool had_connection = current.ip[0] || current.state == State::Connecting;
        if (had_connection) {
            const esp_err_t err = esp_wifi_disconnect();
            if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_CONNECT) {
                error("Disconnect Wi-Fi", err, "Could not switch networks. Try again.");
                return;
            }
        }
        clear_connection();
        joining_after_disconnect = had_connection;

        wifi_config_t config{};
        memcpy(config.sta.ssid, command.network.ssid, sizeof(config.sta.ssid));
        memcpy(config.sta.password, command.password, sizeof(config.sta.password));
        config.sta.threshold.authmode = command.network.secured ? WIFI_AUTH_WPA_PSK : WIFI_AUTH_OPEN;
        config.sta.pmf_cfg.capable = true;
        const esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &config);
        explicit_bzero(&config, sizeof(config));
        if (err != ESP_OK) {
            error("Set network credentials", err, "Could not use this network configuration.");
            return;
        }

        strlcpy(current.ssid, command.network.ssid, sizeof(current.ssid));
        try_connect();
    }

    void try_connect()
    {
        connect_started = xTaskGetTickCount();
        status(State::Connecting, "Joining network…");
        const esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            clear_connection();
            error("Connect Wi-Fi", err, "Could not join the network. Try again.");
        }
    }

    void got_ip(const esp_ip4_addr_t &ip)
    {
        if (!current.enabled || (current.state != State::Connecting && !current.ip[0])) {
            return;
        }
        esp_ip4addr_ntoa(&ip, current.ip, sizeof(current.ip));
        joining_after_disconnect = false;
        reconnects_left = 2;
        status(State::Connected);
        ESP_LOGI(TAG, "Wi-Fi station obtained an IPv4 address");

        if (!esp_sntp_enabled()) {
            setenv("TZ", "CST-8", 1);
            tzset();
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
            esp_sntp_setservername(0, "ntp.aliyun.com");
            esp_sntp_init();
        }
    }

    void disconnected(const Command &command)
    {
        if (!current.enabled || !current.ssid[0] ||
            (command.network.ssid[0] && strcmp(current.ssid, command.network.ssid) != 0)) {
            return;
        }
        if (joining_after_disconnect &&
            (command.reason == WIFI_REASON_ASSOC_LEAVE || command.reason == WIFI_REASON_AUTH_LEAVE)) {
            joining_after_disconnect = false;
            return;
        }

        current.ip[0] = '\0';
        if (reconnects_left > 0) {
            --reconnects_left;
            try_connect();
            return;
        }
        ESP_LOGI(TAG, "Wi-Fi station disconnected (reason %u)", static_cast<unsigned>(command.reason));
        clear_connection();
        switch (command.reason) {
        case WIFI_REASON_NO_AP_FOUND:
        case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
            status(State::Error, "Network not found. Choose a network from the scan results.");
            break;
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            status(State::Error, "Could not authenticate. Check the network password.");
            break;
        default:
            status(State::Error, "Could not join the network. Try again.");
            break;
        }
    }

    void forget_network()
    {
        if (!current.enabled || !start_radio()) {
            return;
        }

        esp_err_t err = esp_wifi_disconnect();
        if (err == ESP_OK || err == ESP_ERR_WIFI_NOT_CONNECT) {
            clear_connection();
            wifi_config_t config{};
            err = esp_wifi_set_config(WIFI_IF_STA, &config);
        }
        if (err != ESP_OK) {
            error("Forget Wi-Fi network", err, "Could not forget the network. Try again.");
            return;
        }
        status(State::Idle);
    }
};

void task(void *arg)
{
    Worker(static_cast<QueueHandle_t>(arg)).run();
}

} // namespace

bool start()
{
    if (commands.load()) {
        return true;
    }
    if (starting.test_and_set()) {
        return false;
    }

    const auto queue = xQueueCreate(12, sizeof(Command));
    if (queue && xTaskCreate(task, "factory_wifi", 8192, queue, 9, nullptr) == pdPASS) {
        commands.store(queue);
    } else {
        if (queue) {
            vQueueDelete(queue);
        }
        ESP_LOGE(TAG, "Could not start Wi-Fi worker");
    }
    starting.clear();
    return commands.load() != nullptr;
}

Snapshot snapshot()
{
    portENTER_CRITICAL(&snapshot_lock);
    const auto result = published;
    portEXIT_CRITICAL(&snapshot_lock);
    return result;
}

bool set_enabled(bool enabled)
{
    return post(enabled ? Action::Enable : Action::Disable);
}

bool scan()
{
    return post(Action::Scan);
}

bool connect(const Network &network, const char *password)
{
    Command command{Action::Connect};
    command.network = network;
    command.network.ssid[sizeof(command.network.ssid) - 1] = '\0';
    if (!command.network.ssid[0]) {
        return false;
    }
    if (password && strlcpy(command.password, password, sizeof(command.password)) >= sizeof(command.password)) {
        explicit_bzero(command.password, sizeof(command.password));
        return false;
    }
    const bool queued = post(commands.load(), command);
    explicit_bzero(command.password, sizeof(command.password));
    return queued;
}

bool forget()
{
    return post(Action::Forget);
}

} // namespace factory_wifi
