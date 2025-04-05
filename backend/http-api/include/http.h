#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <crow.h>
#include <nlohmann/json.hpp>
#include "db.h"

namespace ahohs::http_server {

using json = nlohmann::json;

class HttpServer {
 public:
    explicit HttpServer(ahohs::db::PostgresDB& database);

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) noexcept = default;
    HttpServer& operator=(HttpServer&&) noexcept = default;
    ~HttpServer() = default;

    void run(uint16_t port = 18080);

 private:
    ahohs::db::PostgresDB& database;  // 通过依赖注入 (DI) 的数据库实例

    void setup_routes(crow::App<>& app);

    // RESTful API 路由处理函数，每个路由返回 crow::response 对象
    crow::response handle_get_devices();                             // 查询所有设备：GET /devices
    crow::response handle_get_device(const std::string& device_id);    // 查询单个设备：GET /device/<device_id>
    crow::response handle_create_or_update_device(const crow::request& req);  // 新增/更新设备：POST /device
    crow::response handle_update_device(const crow::request& req, const std::string& device_id); // 更新设备信息：PUT /device/<device_id>
    crow::response handle_delete_device(const std::string& device_id); // 删除设备：DELETE /device/<device_id>
};

}  // namespace ahohs::http_server
