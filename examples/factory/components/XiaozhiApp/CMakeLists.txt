if(CONFIG_FACTORY_ENABLE_XIAOZHI)
    set(xiaozhi_sources
        app_fonts.c
        assets/img_app_xiaozhi.c
        XiaozhiActivationClient.cpp
        XiaozhiActivationPrompt.cpp
        XiaozhiApp.cpp
        XiaozhiAudioProcessor.cpp
        XiaozhiClient.cpp
        XiaozhiLocalization.cpp
        XiaozhiUi.cpp
        third_party/xiaozhi_esp32/ogg_demuxer.cc
    )
    set(xiaozhi_prompts
        assets/prompts/activation.ogg
        assets/prompts/success.ogg
        assets/prompts/0.ogg
        assets/prompts/1.ogg
        assets/prompts/2.ogg
        assets/prompts/3.ogg
        assets/prompts/4.ogg
        assets/prompts/5.ogg
        assets/prompts/6.ogg
        assets/prompts/7.ogg
        assets/prompts/8.ogg
        assets/prompts/9.ogg
    )
endif()

idf_component_register(
    SRCS ${xiaozhi_sources}
    INCLUDE_DIRS "."
    REQUIRES brookesia_core esp_audio_effects
    PRIV_REQUIRES
        app_update
        factory_ui
        product_support
        Settings
        esp_event
        esp_http_client
        esp_audio_codec
        esp-sr
        esp_netif
        esp_xiaozhi
        nvs_flash
        cjson
        efuse
        esp_hw_support
        esp_app_format
        esp_partition
        mbedtls
        mcp-c-sdk
        spi_flash
    EMBED_FILES ${xiaozhi_prompts}
)

if(CONFIG_FACTORY_ENABLE_XIAOZHI)
    # Common Chinese glyphs are mapped from the app image when rendering chat.
    idf_component_get_property(fonts_dir 78__xiaozhi-fonts COMPONENT_DIR)
    target_add_binary_data(${COMPONENT_LIB} "${fonts_dir}/cbin/font_puhui_common_30_4.bin" BINARY)
    target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-missing-field-initializers")
endif()
