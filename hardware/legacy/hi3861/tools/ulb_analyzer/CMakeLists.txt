cmake_minimum_required(VERSION 3.10)
project(ulb-analyzer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(spdlog REQUIRED)

add_executable(ulb-analyzer main.cpp)
target_include_directories(ulb-analyzer PRIVATE spdlog::spdlog)
target_link_libraries(ulb-analyzer PRIVATE spdlog::spdlog)
