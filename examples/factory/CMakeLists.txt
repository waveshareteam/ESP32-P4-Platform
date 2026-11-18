# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
if(NOT DEFINED SDKCONFIG_DEFAULTS)
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.rev3_x.defaults")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-brookesia)

# Match the Knob factory reference: this Boost release predates C++26's
# variadic-comma diagnostics. Other components keep the IDF 6.1 default.
idf_component_get_property(FACTORY_BOOST_LIBRARY espressif__esp-boost COMPONENT_LIB)
target_compile_options(${FACTORY_BOOST_LIBRARY} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++23>")

# The pinned MCP SDK uses a bounded strncpy, which GCC 15 reports at -O2.
# Match the Knob build and keep the diagnostic enabled as a warning.
idf_component_get_property(FACTORY_MCP_LIBRARY espressif__mcp-c-sdk COMPONENT_LIB)
target_compile_options(${FACTORY_MCP_LIBRARY} PRIVATE "-Wno-error=stringop-truncation")
