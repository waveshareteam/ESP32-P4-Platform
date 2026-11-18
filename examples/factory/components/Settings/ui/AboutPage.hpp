#pragma once

#include "esp_brookesia.hpp"
#include "lvgl.h"

namespace esp_brookesia::apps {
    class Settings;
}

namespace esp_brookesia::apps {

class AboutPage : public systems::phone::App {
public:
    static AboutPage *requestInstance(bool use_status_bar = true, bool use_navigation_bar = true);

    AboutPage(bool use_status_bar, bool use_navigation_bar);
    virtual ~AboutPage();

    bool run() override;
    bool back() override;
    bool close() override;

private:
    static AboutPage *_instance;
    lv_obj_t *label;
    lv_obj_t *list1;
};

} // namespace esp_brookesia::apps
