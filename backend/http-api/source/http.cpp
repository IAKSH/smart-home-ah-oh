#include <crow.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <format>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "http.h"

using json = nlohmann::json;

namespace ahohs::http_server {

static auto logger = spdlog::stdout_color_mt("http_server");

// Crow 日志处理器：实现 ILogHandler，将 Crow 日志转发给 spdlog
struct CrowLogHandler : public crow::ILogHandler {
    void log(std::string message, crow::LogLevel level) override {
        switch (level) {
            case crow::LogLevel::Info:
                logger->info(message);
                break;
            case crow::LogLevel::Warning:
                logger->warn(message);
                break;
            case crow::LogLevel::Error:
                logger->error(message);
                break;
            default:
                logger->debug(message);
                break;
        }
    }
};
static CrowLogHandler crow_log_handler;

//////////////////////
// HttpServer 类实现
//////////////////////

HttpServer::HttpServer(ahohs::db::PostgresDB& database)
    : database(database) {
    logger->info("HttpServer initialized.");
}

void HttpServer::setup_routes(crow::App<>& app) {
    // 首页路由：显示硬件线程信息
    CROW_ROUTE(app, "/")
    ([]() {
        return std::format("Hello world!\nThis hardware has {} threads.",
                           std::thread::hardware_concurrency());
    });

    // 索引页面：返回本地 index.html 的内容
    CROW_ROUTE(app, "/index")
    ([]() {
        std::ifstream file("index.html");
        if (!file.is_open()) {
            return std::string("Index page not found.");
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    });

    // RESTful API 路由

    // 查询所有设备：GET /devices
    CROW_ROUTE(app, "/devices").methods("GET"_method)
    ([this]() {
        return this->handle_get_devices();
    });

    // 查询单个设备：GET /device/<device_id>
    CROW_ROUTE(app, "/device/<string>").methods("GET"_method)
    ([this](const std::string& device_id) {
        return this->handle_get_device(device_id);
    });

    // 上传/更新设备元数据：POST /device
    // JSON Body 中需包含 "device_id" 和 "meta" 字段
    CROW_ROUTE(app, "/device").methods("POST"_method)
    ([this](const crow::request& req) {
        return this->handle_create_or_update_device(req);
    });

    // 更新设备信息：PUT /device/<device_id>
    CROW_ROUTE(app, "/device/<string>").methods("PUT"_method)
    ([this](const crow::request& req, const std::string& device_id) {
        return this->handle_update_device(req, device_id);
    });

    // 删除设备：DELETE /device/<device_id>
    CROW_ROUTE(app, "/device/<string>").methods("DELETE"_method)
    ([this](const std::string& device_id) {
        return this->handle_delete_device(device_id);
    });

    CROW_CATCHALL_ROUTE(app)
    ([]() {
        return "404 Not Found";
    });
}

crow::response HttpServer::handle_get_devices() {
    json response;
    auto result_opt = database.query_prepared("get_all_devices", {});
    json device_array = json::array();
    if (result_opt) {
        for (const auto& row : *result_opt) {
            json device;
            device["device_id"] = std::string(row["device_id"].c_str());
            std::string meta_str = std::string(row["meta"].c_str());
            try {
                json meta_json = json::parse(meta_str);
                device["meta"] = meta_json;
            } catch (const std::exception& e) {
                spdlog::error("Failed to parse meta field as JSON: {}", e.what());
                device["meta"] = meta_str;
            }
            device_array.push_back(device);
        }
        response["devices"] = device_array;
    } else {
        response["error"] = "Failed to query devices.";
    }
    crow::response resp(response.dump());
    resp.add_header("Content-Type", "application/json");
    return resp;
}

crow::response HttpServer::handle_get_device(const std::string& device_id) {
    json response;
    auto result_opt = database.query_prepared("get_device", {device_id});
    if (result_opt && result_opt->size() > 0) {
        const auto& row = (*result_opt)[0];
        response["device_id"] = std::string(row["device_id"].c_str());
        std::string meta_str = std::string(row["meta"].c_str());
        try {
            json meta_json = json::parse(meta_str);
            response["meta"] = meta_json;
        } catch (const std::exception& e) {
            spdlog::error("Failed to parse meta field as JSON: {}", e.what());
            response["meta"] = meta_str;
        }
    } else {
        response["error"] = "Device not found.";
    }
    crow::response resp(response.dump());
    resp.add_header("Content-Type", "application/json");
    return resp;
}

crow::response HttpServer::handle_create_or_update_device(const crow::request& req) {
    json response;
    json body = json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        response["error"] = "Invalid JSON input.";
        return crow::response(response.dump());
    }
    // 校验必要字段 device_id 和 meta 是否存在
    if (!body.contains("device_id") || !body.contains("meta")) {
        response["error"] = "Missing required field: device_id or meta.";
        return crow::response(response.dump());
    }
    std::string device_id = body["device_id"].get<std::string>();
    std::string meta;
    if (body["meta"].is_string()) {
        meta = body["meta"].get<std::string>();
    } else {
        meta = body["meta"].dump();
    }
    // 使用数据库接口进行 upsert 操作（新增或更新设备元数据）
    bool success = database.exec_prepared("upsert_device_meta", {device_id, meta});
    response["message"] = success ? "Device added/updated successfully." 
                                  : "Failed to add/update device.";
    crow::response resp(response.dump());
    resp.add_header("Content-Type", "application/json");
    return resp;
}

crow::response HttpServer::handle_update_device(const crow::request& req, const std::string& device_id) {
    json response;
    json body = json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        response["error"] = "Invalid JSON input.";
        return crow::response(response.dump());
    }
    if (!body.contains("meta")) {
        response["error"] = "Missing required field: meta.";
        return crow::response(response.dump());
    }
    std::string meta = body["meta"].is_string() ? body["meta"].get<std::string>() : body["meta"].dump();
    bool success = database.exec_prepared("upsert_device_meta", {device_id, meta});
    response["message"] = success ? "Device updated successfully." 
                                  : "Failed to update device.";
    crow::response resp(response.dump());
    resp.add_header("Content-Type", "application/json");
    return resp;
}

crow::response HttpServer::handle_delete_device(const std::string& device_id) {
    json response;
    bool success = database.exec_prepared("delete_device", {device_id});
    response["message"] = success ? "Device deleted successfully." 
                                  : "Failed to delete device.";
    crow::response resp(response.dump());
    resp.add_header("Content-Type", "application/json");
    return resp;
}

void HttpServer::run(uint16_t port) {
    crow::logger::setHandler(&crow_log_handler);
    crow::App<> app;
    setup_routes(app);
    logger->info("Starting HTTP server on port {}", port);
    app.port(port)
       .multithreaded()
       .run();
    logger->info("HTTP server shutdown.");
}

}  // namespace ahohs::http_server
