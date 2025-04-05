#include <thread>
#include <chrono>
#include "mqtt.h"

namespace ahohs::mqtt_server {

MqttServer::MqttServer(const std::string& server_address,
                       const std::string& client_id,
                       const std::vector<std::string>& topics,
                       ahohs::db::PostgresDB& db)
    : client(server_address, client_id),
      topics(topics),
      db(db) {
    conn_opts.set_clean_session(true);  // 配置清理 session 后自动重连
    callback = std::make_shared<Callback>(*this);
    client.set_callback(*callback);
    logger->info("MqttServer initialized with server: {}", server_address);
}

void MqttServer::start() {
    try {
        logger->info("Connecting to MQTT broker at {}", client.get_server_uri());
        client.connect(conn_opts, nullptr, *callback)->wait();
        // 持续保持 MQTT 客户端运行
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const mqtt::exception& exc) {
        logger->error("Encountered exception: {}", exc.what());
    }
}

// ===== Callback 类实现 =====

MqttServer::Callback::Callback(MqttServer& server)
    : server(server), n_retry(0) {}

void MqttServer::Callback::reconnect() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    try {
        logger->info("Attempting to reconnect...");
        server.client.connect(server.conn_opts, nullptr, *this);
    } catch (const mqtt::exception& exc) {
        logger->error("Reconnection failed: {}", exc.what());
        exit(1);
    }
}

void MqttServer::Callback::connected(const std::string& cause) {
    logger->info("Connected successfully{}", cause.empty() ? "" : (": " + cause));
    // 订阅所有主题
    for (const auto& topic : server.topics) {
        logger->trace("Subscribing to topic: \"{}\"", topic);
        server.client.subscribe(topic, MQTT_QOS, nullptr, *this);
    }
}

void MqttServer::Callback::on_failure(const mqtt::token& token) {
    logger->error("Connection attempt failed.");
    if (++n_retry > N_RETRYATTEMPTS) {
        logger->critical("Exceeded maximum retry attempts. Exiting.");
        exit(1);
    }
    reconnect();
}

void MqttServer::Callback::on_success(const mqtt::token& token) {
    logger->info("Operation succeeded.");
}

void MqttServer::Callback::connection_lost(const std::string& cause) {
    logger->warn("Connection lost{}", cause.empty() ? "" : (" Cause: " + cause));
    n_retry = 0;
    reconnect();
}

void MqttServer::Callback::message_arrived(mqtt::const_message_ptr msg) {
    // 针对 MQTT 收到的消息，不再处理元数据上报（该功能已由 HTTP API 取代）。
    // 如需要处理其它类型的消息，可调用泛化的 process_message 函数或自行添加处理逻辑。
    logger->info("Message arrived on topic: {} (ignored for metadata processing)", msg->get_topic());
}

void MqttServer::Callback::delivery_complete(mqtt::delivery_token_ptr token) {
    logger->info("Delivery complete.");
}

}  // namespace ahohs::mqtt_server
