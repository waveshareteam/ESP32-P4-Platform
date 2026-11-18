set(PROJ_SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE PROJ_SRCS_C ${PROJ_SRC}/*.c)
file(GLOB_RECURSE PROJ_SRCS_CPP ${PROJ_SRC}/*.cpp)
if(NOT (CONFIG_ESP_WIFI_REMOTE_ENABLED AND CONFIG_ESP_HOSTED_ENABLED))
    list(FILTER PROJ_SRCS_CPP EXCLUDE REGEX "/(WlanPage|wifi_service)\\.cpp$")
endif()

idf_component_register(
    SRCS  ${PROJ_SRCS_C} ${PROJ_SRCS_CPP}
    INCLUDE_DIRS ${PROJ_SRC}
    REQUIRES factory_ui esp32_p4_platform driver esp_wifi esp_wifi_remote esp_hosted lwip product_support
    PRIV_REQUIRES esp_app_format esp_hw_support esp_psram spi_flash
)

set_source_files_properties(
    ${PROJ_SRCS_CPP}
    PROPERTIES
        COMPILE_FLAGS "-Wno-missing-field-initializers"
)
