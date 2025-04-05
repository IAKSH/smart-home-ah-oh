#include <thread>
#include <chrono>
#include "mqtt.h"

namespace ahohs::mqtt_server {

MqttServer::MqttServer(const std::string& serveraddress,
                       const std::string& clientid,
                       const std::vector<std::string>& topics,
                       ahohs::db::PostgresDB& db)
    : client(serveraddress, clientid),
      topics(topics),
      db(db) {
    conn_opts.set_clean_session(true);  // 配置清理 session 后自动重连
    callback = std::make_shared<Callback>(*this);
    client.set_callback(*callback);
    logger->info("MqttServer initialized with server: {}", serveraddress);
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

void MqttServer::process_meta_message(const std::string& topic, const std::string& content) {
    const std::string prefix = "/device/";
    const std::string suffix = "/meta";

    // 校验 topic 格式
    if (topic.size() <= prefix.size() + suffix.size() ||
        topic.compare(0, prefix.size(), prefix) != 0 ||
        topic.compare(topic.size() - suffix.size(), suffix.size(), suffix) != 0) {
        logger->warn("Topic '{}' is not a valid meta format.", topic);
        return;
    }

    // 提取设备 ID，例如从 "/device/hi3861_1/meta" 提取 "hi3861_1"
    std::string device_id = topic.substr(prefix.size(), topic.size() - prefix.size() - suffix.size());

    // 准备参数
    std::vector<std::string> params{device_id, content};

    // 使用数据库接口 upsert 设备 meta 信息
    if (db.exec_prepared("upsert_device_meta", params)) {
        logger->info("Successfully recorded meta information for device '{}'.", device_id);
    } else {
        logger->error("Failed to record meta information for device '{}'.", device_id);
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
    logger->info("Message arrived:\n\tTopic: {}\n\tContent: {}",
                 msg->get_topic(), msg->to_string());
    const std::string& topic = msg->get_topic();
    if (topic.size() >= 5 && topic.substr(topic.size() - 5) == "/meta") {
        server.process_meta_message(topic, msg->to_string());
    } else {
        // 泛化消息处理的占位示例，可替换为实际逻辑
        // server.process_message(topic, msg->to_string(), [](const std::string& s) { return s; });
    }
}

void MqttServer::Callback::delivery_complete(mqtt::delivery_token_ptr token) {
    logger->info("Delivery complete.");
}

}  // namespace ahohs::mqtt_server
