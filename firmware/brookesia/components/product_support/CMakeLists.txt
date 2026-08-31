set(SRCS "")
list(APPEND SRCS
    "src/product_support.c"
)

set(INCLUDE_DIRS "")
list(APPEND INCLUDE_DIRS "include")

foreach(SRC_DIR IN LISTS SRC_DIRS)
    file(GLOB_RECURSE SRC ${SRC_DIR}/*.c)
    list(APPEND SRCS ${SRC})
    list(APPEND INCLUDE_DIRS ${SRC_DIR}/include)
endforeach()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES driver esp_codec_dev esp32_p4_platform esp-audio-player esp-file-iterator
    PRIV_REQUIRES esp_timer fatfs esp_psram esp_mm
)

target_compile_options(
    ${COMPONENT_LIB} PRIVATE
        -Wno-format
        -Wno-int-conversion
        -Wno-incompatible-pointer-types
        -Wunused-function
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-overflow
        -Wno-unused-but-set-variable
        -Wno-discarded-qualifiers)
