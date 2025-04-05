#include <iostream>
#include <format>
#include <thread>
#include <exception>
#include <spdlog/spdlog.h>

// 根据你的项目实际情况，引入 HTTP、MQTT 和数据库相关的头文件
#include "http.h"  // 新版 HTTP 服务头文件（使用 nlohmann/json）
#include "mqtt.h"           // MQTT 服务头文件
#include "db.h"             // 数据库接口

#ifndef MQTT_SERVER_ADDRESS
#define MQTT_SERVER_ADDRESS "tcp://192.168.31.110:1883"
#endif

#ifndef PG_CONNECTION_STRING
#define PG_CONNECTION_STRING "host=192.168.31.110 user=postgres password=mysecretpassword dbname=mqttdb port=5432"
#endif

// 下面这个 TITLE 内容可根据需要调整
static const std::string_view TITLE { R"(
 █████╗ ██╗  ██╗ ██████╗ ██╗  ██╗       █████╗ ██████╗ ██╗      ███████╗███████╗██████╗ ██╗   ██╗███████╗██████╗ 
██╔══██╗██║  ██║██╔═══██╗██║  ██║      ██╔══██╗██╔══██╗██║      ██╔════╝██╔════╝██╔══██╗██║   ██║██╔════╝██╔══██╗
███████║███████║██║   ██║███████║█████╗███████║██████╔╝██║█████╗███████╗█████╗  ██████╔╝██║   ██║█████╗  ██████╔╝
██╔══██║██╔══██║██║   ██║██╔══██║╚════╝██╔══██║██╔═══╝ ██║╚════╝╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝██╔══╝  ██╔══██╗
██║  ██║██║  ██║╚██████╔╝██║  ██║      ██║  ██║██║     ██║      ███████║███████╗██║  ██║ ╚████╔╝ ███████╗██║  ██║
)" };

int main() {
    try {
        // 输出标题
        std::cout << TITLE << "\n";

        // 创建数据库实例，使用项目中定义的连接字符串宏 PG_CONNECTION_STRING
        ahohs::db::PostgresDB database(PG_CONNECTION_STRING);

        // 由外部统一注册所有预处理语句，避免重复注册问题
        try {
            database.register_prepared_statement("upsert_device_meta",
                "INSERT INTO devices (device_id, meta) VALUES ($1, $2) "
                "ON CONFLICT (device_id) DO UPDATE SET meta = EXCLUDED.meta, updated_at = CURRENT_TIMESTAMP;");
            database.register_prepared_statement("delete_device",
                "DELETE FROM devices WHERE device_id = $1;");
            database.register_prepared_statement("get_device",
                "SELECT device_id, meta FROM devices WHERE device_id = $1;");
            database.register_prepared_statement("get_all_devices",
                "SELECT device_id, meta FROM devices;");
        } catch (const std::exception &ex) {
            spdlog::error("Register prepared statements failed: {}", ex.what());
            return 1;
        }

        // 创建 HTTP 服务实例，并传入数据库引用
        ahohs::http_server::HttpServer httpServer(database);

        // 创建 MQTT 服务实例；MQTT_TOPICS 宏预定义为 e.g. {"/device/#"}
        std::vector<std::string> topics = MQTT_TOPICS;
        ahohs::mqtt_server::MqttServer mqttServer(MQTT_SERVER_ADDRESS, MQTT_CLIENT_ID, topics, database);

        // 分别启动 HTTP 和 MQTT 服务线程，使用多线程使它们并行运行
        std::thread httpThread([&httpServer]() {
            httpServer.run();  // 默认端口为 18080
        });

        std::thread mqttThread([&mqttServer]() {
            mqttServer.start();  // 此函数内部保持连接，并持续运行
        });

        // 主线程等待两个服务线程结束（通常服务为长驻服务）
        httpThread.join();
        mqttThread.join();
    }
    catch (const std::exception &ex) {
        spdlog::error("Exception occurred in main: {}", ex.what());
        return 1;
    }
    return 0;
}
