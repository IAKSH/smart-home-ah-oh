# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(hmos_cpp_test)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

set(PAHO_BUILD_DOCUMENTATION OFF CACHE BOOL "Disable docs build")
set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "Disable samples build")
set(PAHO_WITH_MQTT_C ON CACHE BOOL "Build Paho C from the internal GIT submodule." FORCE)
set(PAHO_WITH_SSL OFF CACHE BOOL "Build SSL-enabled library" FORCE)

add_subdirectory(external/paho.mqtt.cpp)

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include
                    ${NATIVERENDER_ROOT_PATH}/external/asio/asio/include
)

add_library(entry SHARED
    napi_init.cpp
    api.cpp
    http_fetch.cpp
    udp_broadcast.cpp
    mqtt.cpp
    mqtt_napi.cpp
)

add_dependencies(entry PahoMqttCpp::paho-mqttpp3-shared)

set_target_properties(entry PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(entry PUBLIC libace_napi.z.so libhilog_ndk.z.so PahoMqttCpp::paho-mqttpp3-shared)