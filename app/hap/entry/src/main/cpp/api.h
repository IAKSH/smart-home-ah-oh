//
// Created on 2025/4/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HMOS_CPP_TEST_API_H
#define HMOS_CPP_TEST_API_H

#include <string>
#include <string_view>

namespace ahohc::api {
    void connect_to_server(const std::string_view& ip,const std::string_view& port);
    void delete_device(const std::string_view& device_id);
    std::string fetch_devices();
    std::string fetch_device(const std::string_view& device_id);

    void init_udp_context();
    std::string discover_server();
}

#endif //HMOS_CPP_TEST_API_H
