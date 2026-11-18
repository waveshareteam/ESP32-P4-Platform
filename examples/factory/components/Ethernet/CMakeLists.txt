set(sources)
if(CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET)
    set(sources "Ethernet.cpp" "ethernet_service.cpp")
endif()

idf_component_register(
    SRCS ${sources}
    INCLUDE_DIRS "."
    REQUIRES brookesia_core
    PRIV_REQUIRES factory_ui esp32_p4_platform ethernet_init esp_eth esp_event esp_netif
)

if(sources)
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
endif()
