//
// Created on 2025/4/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "mqtt.h"
#include "mqtt/message.h"
#include "utils/log.h"

#define TAG "mqtt"

namespace ahohc::mqtt {

Context::Context(const std::string_view& broker, const std::string_view& clientId)
    : client(std::string(broker), std::string(clientId)),
      conn_opts(),
      callback(this),
      broker(broker)
{
    // 可根据实际需求调整连接选项
    conn_opts.set_keep_alive_interval(20);
    conn_opts.set_clean_session(true);
    client.set_callback(callback);
}

Context::~Context() {
    try {
        disconnect();
    } catch (...) { /* 忽略析构阶段异常 */ }
}

void Context::connect() {
    // 连接成功后等待返回
    client.connect(conn_opts)->wait();
    AHOHC_LOG(LOG_INFO, TAG, "mqtt broker %{public}s connected",broker.c_str());
}

void Context::disconnect() {
    client.disconnect()->wait();
    AHOHC_LOG(LOG_INFO, TAG, "mqtt broker %{public}s disconnected",broker.c_str());
}

void Context::subscribe(const std::string_view& topic, int qos) {
    client.subscribe(std::string(topic), qos)->wait();
    AHOHC_LOG(LOG_INFO, TAG, "subscribed topic %{public}s from broker %{public}s",topic.data(),broker.c_str());
}

void Context::publish(const std::string_view& topic, const std::string_view& payload,
                      int qos, bool retained) {
    auto pubmsg = ::mqtt::make_message(std::string(topic), std::string(payload));
    pubmsg->set_qos(qos);
    pubmsg->set_retained(retained);
    client.publish(pubmsg)->wait();
    AHOHC_LOG(LOG_INFO, TAG, "published topic %{public}s to broker %{public}s",topic.data(),broker.c_str());
}

void Context::set_message_callback(MessageCallback cb) {
    message_callback = std::move(cb);
}

void Context::Callback::message_arrived(::mqtt::const_message_ptr msg) {
    if (context && context->message_callback) {
        context->message_callback(msg->get_topic(),msg->to_string());
    }
}
}
