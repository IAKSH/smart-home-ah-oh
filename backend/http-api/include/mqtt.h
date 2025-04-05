#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <chrono>
#include <mqtt/async_client.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "db.h"

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "paho_cpp_demo_client"
#endif

#ifndef MQTT_TOPICS
#define MQTT_TOPICS {"/device/#"}
#endif

#ifndef MQTT_QOS
#define MQTT_QOS 1
#endif

#ifndef MQTT_N_RETRY_ATTEMPTS
#define MQTT_N_RETRY_ATTEMPTS 5
#endif

namespace ahohs::mqtt_server {

class MqttServer {
 public:
    MqttServer(const std::string& server_address,
               const std::string& client_id,
               const std::vector<std::string>& topics,
               ahohs::db::PostgresDB& db);

    ~MqttServer() = default;
    void start();
    void process_meta_message(const std::string& topic, const std::string& content);

    /**
     * 模板化消息处理函数
     *
     * 此函数用于泛化消息解析与转换。允许用户传递自定义转换器（如 lambda 函数或函数对象），
     * 它将原始消息内容转换为目标类型 T。
     *
     * @tparam Converter 自定义转换器类型
     * @tparam T 目标类型（通过 std::invoke_result_t 推导）
     * @param topic 消息所属的主题
     * @param content 消息内容
     * @param converter 用于消息转换的函数对象，签名为 T(const std::string&)
     */
    template <typename Converter, typename T = std::invoke_result_t<Converter, const std::string&>>
    void process_message(const std::string& topic,
                         const std::string& content,
                         Converter converter) {
        T result = converter(content);
        // 示例：日志记录转换结果
        logger->info("Processed message on topic {}. Converted result type: {}", topic, typeid(T).name());
    }

 private:
    mqtt::async_client client;
    mqtt::connect_options conn_opts;
    std::vector<std::string> topics;

    ahohs::db::PostgresDB& db;

    static constexpr int N_RETRYATTEMPTS = 3;

    /**
     * 嵌套回调类
     *
     * 用于处理 MQTT 的各种回调事件。
     */
    class Callback : public mqtt::callback, public mqtt::iaction_listener {
     public:
        explicit Callback(MqttServer& server);

        // mqtt::callback 的重载
        void connected(const std::string& cause) override;
        void connection_lost(const std::string& cause) override;
        void message_arrived(mqtt::const_message_ptr msg) override;
        void delivery_complete(mqtt::delivery_token_ptr token) override;

        // mqtt::iaction_listener 的重载
        void on_failure(const mqtt::token& token) override;
        void on_success(const mqtt::token& token) override;

     private:
        void reconnect();  // 断线重连

        MqttServer& server;  // 外部服务器引用
        int n_retry;  // 重试次数
        inline static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("mqtt_callback");
    };

    std::shared_ptr<Callback> callback;  // 回调对象
    inline static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("mqtt_server");
};

}  // namespace ahohs::mqtt_server
