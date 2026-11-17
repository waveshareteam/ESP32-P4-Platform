#pragma once

#include "esp_brookesia.hpp"
#include "lvgl.h"
#include <vector>
#include "esp_mac.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"

namespace esp_brookesia::apps {
    class Settings;
}

namespace esp_brookesia::apps {

class WlanPage : public systems::phone::App {
public:
    enum WifiState {
        WIFIOPEN,
        WIFICLOSE,
        WIFISCAN,
        CONNECTED,
        CONNECTING,
        DISCONNECT,
    };


    static WlanPage *requestInstance(bool use_status_bar = true, bool use_navigation_bar = true);

    WlanPage(bool use_status_bar, bool use_navigation_bar);
    virtual ~WlanPage();

    bool run() override;
    bool back() override;
    bool close() override;

    void toggleWifiUI(WifiState visible);

private:

    typedef struct {
        int index;
        void *self;
    } btn_user_data_t;

    // Default scan list size (max 5 APs)
    #define DEFAULT_SCAN_LIST_SIZE 5 // 0 ~ 5

    static WlanPage *_instance;
    int wifi_status_icon_state = 0;

    lv_obj_t *label;
    lv_obj_t *list1;
    lv_obj_t *status_btn;
    lv_obj_t *wlan_switch;
    lv_obj_t *wifi_icon;

    lv_obj_t *spinner;
    static lv_obj_t *ta;
    static lv_obj_t *kb;
    

    // UI controls that are updated from scan/connect callbacks.
    lv_obj_t *connected_text;
    lv_obj_t *conn_btn;
    lv_obj_t *available_text;
    std::vector<lv_obj_t *> wifi_btns;

    // Styles are owned by the page and reset when the page closes.
    lv_style_t style_list;
    lv_style_t style_list_btn;
    lv_style_t style_list_text;
    lv_style_t style_list_btn_pressed;

    // Wi-Fi state machine and background scan task.
    TaskHandle_t wifi_TaskHandle;
    bool wifi_task_running = false;
    bool page_active = false;

    bool WIFI_STA_FLAG = false;
    bool WIFI_AP_FLAG = false;

    WifiState Wifi_state = WIFICLOSE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t scanned_ap_count = 0;
    esp_netif_t *sta_netif = NULL;

    int wifi_index;
    uint8_t *wifi_ssid;
    uint8_t *wifi_pwd;
    uint8_t connection_num = 0;

    void CreateWifiUI();
    static void wifi_scan_cb(lv_timer_t * timer);
    static void wifi_sta_cb(lv_timer_t * timer);
    static void wifi_btn_cb(lv_event_t * e);

    static void wifi_task(void *arg);

    static void wifi_state_cb(void *param);
    static void wifi_event_handler(void *arg, esp_event_base_t event_base,\
                                int32_t event_id, void *event_data);

    bool wifi_events_registered = false;                            
    void start_wifi_events();
    void stop_wifi_events();
    
    bool OpenWifi();
    bool CloseWifi();
    bool ConnWifi();

    // Password input and keyboard helpers.
    static void adjust_content_for_keyboard();
    static void restore_content_padding();
    static void kb_event_cb(lv_event_t *e);
    static void ta_event_cb(lv_event_t *e);
    static void connection_event_cb(lv_event_t * e);
    static void cancel_event_cb(lv_event_t * e);
    static lv_obj_t* create_labeled_ta(lv_obj_t *parent, const char *label_txt, const char *placeholder);

    // NVS keys for page-level settings.
    #define NVS_STORAGE_NAMESPACE           "storage"
    #define NVS_KEY_WIFI_ENABLE             "wifi_en"

    std::map<std::string, int32_t> _nvs_param_map;

    bool loadNvsParam(void);
    bool setNvsParam(std::string key, int value);
};

} // namespace esp_brookesia::apps
