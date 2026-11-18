/*
 * SPDX-FileCopyrightText: 2026 Waveshare
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ethernet_service.hpp"

#include <atomic>
#include <cstdio>
#include "esp_check.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "ethernet_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace factory_ethernet {
namespace {

constexpr char TAG[] = "factory_eth";
std::atomic_flag started = ATOMIC_FLAG_INIT;
portMUX_TYPE snapshot_lock = portMUX_INITIALIZER_UNLOCKED;
Snapshot published{};

struct Interface {
    esp_eth_handle_t *drivers = nullptr;
    uint8_t driver_count = 0;
    esp_netif_t *netif = nullptr;
    esp_eth_netif_glue_handle_t glue = nullptr;
    esp_event_handler_instance_t link_handler = nullptr;
    esp_event_handler_instance_t ip_handler = nullptr;
    bool start_attempted = false;
};

Interface ethernet;

void publish(Snapshot current)
{
    portENTER_CRITICAL(&snapshot_lock);
    current.revision = published.revision + 1;
    published = current;
    portEXIT_CRITICAL(&snapshot_lock);
}

void link_event(void *arg, esp_event_base_t, int32_t id, void *data)
{
    if (!data) return;
    const auto &interface = *static_cast<Interface *>(arg);
    auto driver = *static_cast<esp_eth_handle_t *>(data);
    if (driver != interface.drivers[0]) return;

    auto current = snapshot();
    if (id == ETHERNET_EVENT_CONNECTED) {
        eth_speed_t speed;
        eth_duplex_t duplex;
        if (esp_eth_ioctl(driver, ETH_CMD_G_SPEED, &speed) == ESP_OK &&
            esp_eth_ioctl(driver, ETH_CMD_G_DUPLEX_MODE, &duplex) == ESP_OK) {
            const int mbps = speed == ETH_SPEED_1000M ? 1000 : speed == ETH_SPEED_100M ? 100 : 10;
            std::snprintf(current.link, sizeof(current.link), "%d Mb/s, %s duplex",
                          mbps, duplex == ETH_DUPLEX_FULL ? "full" : "half");
        }
        current.state = State::ObtainingAddress;
        ESP_LOGI(TAG, "Link up (%s)", current.link);
    } else if (id == ETHERNET_EVENT_DISCONNECTED || id == ETHERNET_EVENT_STOP) {
        current.state = State::NoLink;
        current.link[0] = '\0';
        current.address[0] = '\0';
        current.gateway[0] = '\0';
        ESP_LOGI(TAG, "Link down");
    } else {
        return;
    }
    publish(current);
}

void ip_event(void *arg, esp_event_base_t, int32_t id, void *data)
{
    if (!data || (id != IP_EVENT_ETH_GOT_IP && id != IP_EVENT_ETH_LOST_IP)) return;
    const auto &interface = *static_cast<Interface *>(arg);
    const auto &event = *static_cast<const ip_event_got_ip_t *>(data);
    if (event.esp_netif != interface.netif) return;

    auto current = snapshot();
    if (current.state != State::Connected && current.state != State::ObtainingAddress) return;
    if (id == IP_EVENT_ETH_GOT_IP) {
        esp_ip4addr_ntoa(&event.ip_info.ip, current.address, sizeof(current.address));
        if (event.ip_info.gw.addr) {
            esp_ip4addr_ntoa(&event.ip_info.gw, current.gateway, sizeof(current.gateway));
        } else {
            current.gateway[0] = '\0';
        }
        current.state = State::Connected;
        ESP_LOGI(TAG, "DHCP IPv4 %s (gateway %s)", current.address, current.gateway);
    } else {
        current.state = State::ObtainingAddress;
        current.address[0] = '\0';
        current.gateway[0] = '\0';
        ESP_LOGI(TAG, "Lost DHCP IPv4 address");
    }
    publish(current);
}

esp_err_t initialize(Interface &interface)
{
    ESP_RETURN_ON_ERROR(example_eth_init(&interface.drivers, &interface.driver_count), TAG, "Install driver");
    ESP_RETURN_ON_FALSE(interface.driver_count == 1, ESP_ERR_INVALID_STATE, TAG, "Expected one Ethernet device");
    auto driver = interface.drivers[0];

    esp_netif_config_t config = ESP_NETIF_DEFAULT_ETH();
    interface.netif = esp_netif_new(&config);
    ESP_RETURN_ON_FALSE(interface.netif, ESP_ERR_NO_MEM, TAG, "Create Ethernet interface");
    interface.glue = esp_eth_new_netif_glue(driver);
    ESP_RETURN_ON_FALSE(interface.glue, ESP_FAIL, TAG, "Create Ethernet network glue");
    ESP_RETURN_ON_ERROR(esp_netif_attach(interface.netif, interface.glue), TAG, "Attach Ethernet interface");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(
        ETH_EVENT, ESP_EVENT_ANY_ID, link_event, &interface, &interface.link_handler), TAG, "Register link events");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID, ip_event, &interface, &interface.ip_handler), TAG, "Register address events");

    Snapshot current{.state = State::NoLink};
    uint8_t mac[ETH_ADDR_LEN];
    if (esp_eth_ioctl(driver, ETH_CMD_G_MAC_ADDR, mac) == ESP_OK) {
        std::snprintf(current.mac, sizeof(current.mac), MACSTR, MAC2STR(mac));
    }
    publish(current);
    interface.start_attempted = true;
    return esp_eth_start(driver);
}

void cleanup(Interface &interface)
{
    if (interface.ip_handler) esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, interface.ip_handler);
    if (interface.link_handler) esp_event_handler_instance_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, interface.link_handler);
    if (interface.start_attempted) esp_eth_stop(interface.drivers[0]);
    if (interface.glue) esp_eth_del_netif_glue(interface.glue);
    if (interface.drivers) example_eth_deinit(interface.drivers, interface.driver_count);
    if (interface.netif) esp_netif_destroy(interface.netif);
    interface = {};
}

void start_task(void *)
{
    const esp_err_t err = initialize(ethernet);
    if (err != ESP_OK) {
        cleanup(ethernet);
        ESP_LOGE(TAG, "Unable to start Ethernet: %s", esp_err_to_name(err));
        publish(Snapshot{.state = State::Error});
        started.clear();
    }
    vTaskDelete(nullptr);
}

} // namespace

Snapshot snapshot()
{
    portENTER_CRITICAL(&snapshot_lock);
    const auto current = published;
    portEXIT_CRITICAL(&snapshot_lock);
    return current;
}

bool start()
{
    if (started.test_and_set()) return true;
    publish(Snapshot{});
    if (xTaskCreate(start_task, "factory_eth", 4096, nullptr, 5, nullptr) == pdPASS) return true;

    publish(Snapshot{.state = State::Error});
    started.clear();
    return false;
}

} // namespace factory_ethernet
