# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
include(${CMAKE_CURRENT_LIST_DIR}/../../../config/esp32p4_local_defaults.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_esp_lcd_jd9365)
