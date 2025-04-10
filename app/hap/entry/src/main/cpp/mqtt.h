//
// Created on 2025/4/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HMOS_CPP_TEST_MQTT_H
#define HMOS_CPP_TEST_MQTT_H

#include <mqtt/async_client.h>
#include <string>
#include <string_view>
#include <functional>

namespace ahohc::mqtt {

class Context {
public:
    // 禁止复制
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    // 构造函数传入 broker 地址和客户端ID
    Context(const std::string_view& broker, const std::string_view& clientId);
    ~Context();

    // 连接/断开服务（同步等待连接结果）
    void connect();
    void disconnect();

    // 订阅指定主题，默认为 QOS 1
    void subscribe(const std::string_view& topic, int qos = 1);

    // 发布消息到指定主题，指定 QOS 和 retain 标志
    void publish(const std::string_view& topic, const std::string_view& payload,
                 int qos = 1, bool retained = false);

    // 设置接收到 MQTT 消息时的回调（消息以 std::string 传出）
    using MessageCallback = std::function<void(const std::string_view&,const std::string_view&)>;
    void set_message_callback(MessageCallback cb);

private:
    ::mqtt::async_client client;
    ::mqtt::connect_options conn_opts;
    
    std::string broker;

    // 用户设置的消息回调
    MessageCallback message_callback;

    // 内部回调类，用于接收 paho.mqtt.cpp 的事件
    class Callback : public virtual ::mqtt::callback {
    public:
        Callback(Context* context) : context(context) {}
        void message_arrived(::mqtt::const_message_ptr msg) override;
        void connection_lost(const std::string& cause) override {}
        void delivery_complete(::mqtt::delivery_token_ptr token) override {}
    private:
        Context* context;
    };

    Callback callback;
};
}

#endif //HMOS_CPP_TEST_MQTT_H
