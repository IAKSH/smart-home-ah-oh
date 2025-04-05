#include <iostream>
#include <format>
#include <thread>
#include <exception>
#include <spdlog/spdlog.h>

#include "http.h"    // HTTP 服务模块
#include "mqtt.h"    // MQTT 服务模块
#include "udp.h"     // UDP 响应模块
#include "db.h"      // 数据库接口

#ifndef HTTP_SERVER_PORT
#define HTTP_SERVER_PORT 18080
#endif

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1883
#endif

#ifndef MQTT_SERVER_ADDRESS
#define MQTT_SERVER_ADDRESS "tcp://192.168.31.110:1883"
#endif

#ifndef PG_CONNECTION_STRING
#define PG_CONNECTION_STRING "host=192.168.31.110 user=postgres password=mysecretpassword dbname=mqttdb port=5432"
#endif

#ifndef AUTO_DISCOVERY_SERVER_IP
#define AUTO_DISCOVERY_SERVER_IP "192.168.31.110"
#endif

#ifndef AUTO_DISCOVERY_MQTT_BROKER_IP
#define AUTO_DISCOVERY_MQTT_BROKER_IP "192.168.31.110"
#endif

// APP 启动时展示的标题
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

        // 创建数据库实例，使用项目中定义的连接字符串
        ahohs::db::PostgresDB database(PG_CONNECTION_STRING);

        // 统一注册所有预处理语句，避免重复注册
        try {
            database.register_prepared_statement(
                "upsert_device_meta",
                "INSERT INTO devices (device_id, meta) VALUES ($1, $2) "
                "ON CONFLICT (device_id) DO UPDATE SET meta = EXCLUDED.meta, updated_at = CURRENT_TIMESTAMP;");
            database.register_prepared_statement(
                "delete_device",
                "DELETE FROM devices WHERE device_id = $1;");
            database.register_prepared_statement(
                "get_device",
                "SELECT device_id, meta FROM devices WHERE device_id = $1;");
            database.register_prepared_statement(
                "get_all_devices",
                "SELECT device_id, meta FROM devices;");
        } catch (const std::exception &ex) {
            spdlog::error("Register prepared statements failed: {}", ex.what());
            return 1;
        }

        // 创建 HTTP 服务实例
        ahohs::http_server::HttpServer http_server(database);

        // 创建 MQTT 服务实例
        std::vector<std::string> topics = MQTT_TOPICS;
        ahohs::mqtt_server::MqttServer mqtt_server(MQTT_SERVER_ADDRESS, MQTT_CLIENT_ID, topics, database);

        // 创建 UDP 响应器实例
        // 使用新的 API：传入服务器 IP 和 HTTP 端口、MQTT Broker IP 和 MQTT Broker 端口，以及 UDP监听端口（此处为8888）
        ahohs::udp_server::UdpResponder udp_responder(AUTO_DISCOVERY_SERVER_IP, HTTP_SERVER_PORT, AUTO_DISCOVERY_MQTT_BROKER_IP, MQTT_BROKER_PORT, 8888);

        // 分别启动 HTTP、MQTT 和 UDP 服务线程，使它们并行运行
        std::thread http_thread([&http_server]() {
            http_server.run();  // 默认 HTTP 端口为 18080
        });

        std::thread mqtt_thread([&mqtt_server]() {
            mqtt_server.start();  // 内部保持连接并持续运行
        });

        std::thread udp_thread([&udp_responder]() {
            udp_responder.start();  // 循环监听 UDP 广播消息并进行回复
        });

        // 主线程等待各子线程结束（通常服务为长驻服务）
        http_thread.join();
        mqtt_thread.join();
        udp_thread.join();
    }
    catch (const std::exception &ex) {
        spdlog::error("Exception occurred in main: {}", ex.what());
        return 1;
    }
    return 0;
}
