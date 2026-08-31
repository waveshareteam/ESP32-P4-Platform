idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        product_support
        brookesia_core
    PRIV_REQUIRES
        app_update
        nvs_flash
        esp_wifi
        draw
        SpecAnalyzer
        MusicPlayer
        Settings
        Camera
        VideoPlayer
        brookesia_app_calculator
)

target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-missing-field-initializers)

# Function to get component library
function(get_component_library component_name output_var)
    # Get the exact component name
    idf_build_get_property(build_components BUILD_COMPONENTS)
    set(TARGET_COMPONENT "")
    foreach(COMPONENT ${build_components})
        if(COMPONENT MATCHES "${component_name}" OR COMPONENT MATCHES "espressif__${component_name}")
            set(TARGET_COMPONENT ${COMPONENT})
            break()
        endif()
    endforeach()

    # Get the component library
    if(TARGET_COMPONENT STREQUAL "")
        message(FATAL_ERROR "Component '${component_name}' not found.")
    else()
        idf_component_get_property(COMPONENT_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
        set(${output_var} ${COMPONENT_LIB} PARENT_SCOPE)
    endif()
endfunction()

# get_component_library("lvgl" LVGL_LIB)
# target_compile_options(
#     ${LVGL_LIB}
#     PUBLIC
#         "-Wno-attributes"
# )

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
