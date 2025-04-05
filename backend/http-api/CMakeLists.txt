cmake_minimum_required(VERSION 3.10.0)
project(ahoh VERSION 0.1.0 LANGUAGES CXX)

find_package(spdlog REQUIRED)

set(CROW_USE_BOOST ON)
set(PAHO_BUILD_DOCUMENTATION OFF CACHE BOOL "Disable docs build")
set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "Disable samples build")
set(PAHO_WITH_MQTT_C ON CACHE BOOL "Build Paho C from the internal GIT submodule." FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static linking" FORCE)
# 强制构建 paho MQTT C 库的静态版本
set(PAHO_BUILD_STATIC ON CACHE BOOL "Build static version of Paho MQTT C library" FORCE)
set(PAHO_BUILD_SHARED OFF CACHE BOOL "Build shared version of Paho MQTT C library" FORCE)

add_subdirectory(vendor/Crow)
add_subdirectory(vendor/paho.mqtt.cpp)
add_subdirectory(vendor/json)
add_subdirectory(vendor/libpqxx)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS source/*.cpp include/*.hpp)
add_executable(ahoh-http-server ${sources})
set_target_properties(ahoh-http-server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(ahoh-http-server PUBLIC
    include
    #spdlog::spdlog
    #vendor/Crow/include
    #vendor/paho.mqtt.cpp/include
)
add_dependencies(ahoh-http-server Crow paho-mqttpp3-static)
target_link_libraries(ahoh-http-server PUBLIC spdlog::spdlog Crow paho-mqttpp3-static nlohmann_json::nlohmann_json pqxx)
add_compile_definitions(ahoh-http-server CROW_USE_BOOST)

execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/templates ${CMAKE_CURRENT_LIST_DIR}/../../build/templates)
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/static ${CMAKE_CURRENT_LIST_DIR}/../../build/static)
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/tls ${CMAKE_CURRENT_LIST_DIR}/../../build/tls)