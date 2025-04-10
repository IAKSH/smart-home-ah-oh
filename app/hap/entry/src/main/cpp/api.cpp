//
// Created on 2025/4/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include <memory>
#include <string>
#include <sstream>
#include "api.h"
#include "http_fetch.h"
#include "udp_broadcast.h"

namespace ahohc::api {
static constexpr char* TAG = "ahohc_api";
static std::unique_ptr<ahohc::http::Context> http_context;
static std::unique_ptr<ahohc::udp::Context> udp_context;

static bool has_http_context() {
    if(http_context == nullptr) {
        AHOHC_LOG(LOG_ERROR,TAG,"no http context");
        return false;
    }
    return true;
}

static std::string device_url(const std::string_view& device_id) {
    std::stringstream ss;
    ss << "/api/device/" << device_id;
    return ss.str();
}

void connect_to_server(const std::string_view &ip, const std::string_view &port) {
    try {
        http_context = std::make_unique<ahohc::http::Context>(ip,port);
    }
    catch(std::exception& e) {
        AHOHC_LOG(LOG_ERROR, TAG, "failed to connect to sever %{public}s:%{public}s",ip,port);
    }
}

std::string fetch_devices() {
    if(!has_http_context())
        return "";
    return http_context->http_get("/api/devices");
}

std::string fetch_device(const std::string_view& device_id) {
    if(!has_http_context())
        return "";
    return http_context->http_get(device_url(device_id));
}

void delete_device(const std::string_view& device_id) {
    if(!has_http_context())
        return;
    http_context->http_delete(device_url(device_id));
}

void init_udp_context() {
    udp_context = std::make_unique<ahohc::udp::Context>("192.168.177.255",8888);
    //udp_context = std::make_unique<ahohc::udp::Context>("255.255.255.255",8888);
}

std::string discover_server() {
    if(udp_context == nullptr) {
        AHOHC_LOG(LOG_ERROR, TAG, "no udp context");
        return "";
    }
    udp_context->send("AHOH_DISCOVER_SERVER");
    return udp_context->receive(std::chrono::milliseconds(5000));
}
}