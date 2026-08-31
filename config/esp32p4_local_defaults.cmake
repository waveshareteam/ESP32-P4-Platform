# Apply the supported ESP32-P4 v3.x profile unless the caller selected defaults.
if(
    (NOT DEFINED SDKCONFIG_DEFAULTS OR SDKCONFIG_DEFAULTS STREQUAL "")
    AND
    (NOT DEFINED ENV{SDKCONFIG_DEFAULTS} OR "$ENV{SDKCONFIG_DEFAULTS}" STREQUAL "")
)
    set(
        SDKCONFIG_DEFAULTS
        "sdkconfig.defaults;${CMAKE_CURRENT_LIST_DIR}/esp32p4_rev3_x.defaults"
    )
endif()
