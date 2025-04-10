//
// Created on 2025/4/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include <string>
#include "mqtt_napi.h"

// 全局 MQTT context 和线程安全函数变量
std::unique_ptr<ahohc::mqtt::Context> mqtt_context = nullptr;
napi_threadsafe_function tsfn = nullptr;

// 定义传递消息数据的结构体
struct MessageData {
    std::string topic;
    std::string payload;
};

// 主线程中的回调函数，将 native 信息转换为 JS 字符串后调用注册的 ArkTS 回调
static void call_js_callback(napi_env env, napi_value js_callback, void* context, void* data) {
    MessageData* msg_data = reinterpret_cast<MessageData*>(data);
    if (env != nullptr) {
        napi_value topic, payload;
        napi_create_string_utf8(env, msg_data->topic.data(), NAPI_AUTO_LENGTH, &topic);
        napi_create_string_utf8(env, msg_data->payload.data(), NAPI_AUTO_LENGTH, &payload);
        napi_value argv[2] = {topic,payload};
        napi_call_function(env, nullptr, js_callback, 2, argv, nullptr);
    }
    delete msg_data;
}

// NAPI 接口：注册 ArkTS 回调，用于接收 MQTT 消息
napi_value register_mqtt_callback(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value jsCallback;
    napi_get_cb_info(env, info, &argc, &jsCallback, nullptr, nullptr);

    napi_value resourceName;
    napi_create_string_utf8(env, "MQTTCallbackResource", NAPI_AUTO_LENGTH, &resourceName);

    napi_status status = napi_create_threadsafe_function(
        env,
        jsCallback,       // JavaScript 回调函数
        nullptr,
        resourceName,
        0,                // 队列大小（0 表示无限制）
        1,                // 初始线程数
        nullptr,
        nullptr,          // finalizer（可选）
        nullptr,
        call_js_callback,   // 主线程中实际回调执行函数
        &tsfn
    );
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "无法创建线程安全函数");
    }

    // 设置全局 MQTT context 的消息回调，消息到达时会调用 MqttMessageCallback
    if (mqtt_context) {
        mqtt_context->set_message_callback([](const std::string_view& topic,const std::string_view payload){
            MessageData* data = new MessageData{std::string(topic),std::string(payload)};
            napi_call_threadsafe_function(tsfn, data, napi_tsfn_nonblocking);
        });
    }
    return nullptr;
}

// NAPI 接口：订阅指定主题
napi_value subscribe(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value arg;
    napi_get_cb_info(env, info, &argc, &arg, nullptr, nullptr);

    // 获取传入的主题字符串
    size_t str_size;
    napi_get_value_string_utf8(env, arg, nullptr, 0, &str_size);
    std::string topic;
    topic.resize(str_size);
    napi_get_value_string_utf8(env, arg, &topic[0], str_size + 1, &str_size);

    if (mqtt_context) {
        mqtt_context->connect();
        mqtt_context->subscribe(topic);
    } else {
        napi_throw_error(env, nullptr, "MQTT context 未初始化");
    }
    return nullptr;
}

// NAPI 接口：发布消息到指定主题，参数：topic, payload
napi_value publish(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 2) {
        napi_throw_error(env, nullptr, "需要提供主题和消息内容");
        return nullptr;
    }

    // 获取主题字符串
    size_t topic_size;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &topic_size);
    std::string topic;
    topic.resize(topic_size);
    napi_get_value_string_utf8(env, args[0], &topic[0], topic_size + 1, &topic_size);

    // 获取 payload 字符串
    size_t payload_size;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &payload_size);
    std::string payload;
    payload.resize(payload_size);
    napi_get_value_string_utf8(env, args[1], &payload[0], payload_size + 1, &payload_size);

    if (mqtt_context) {
        mqtt_context->publish(topic, payload);
    } else {
        napi_throw_error(env, nullptr, "MQTT context 未初始化");
    }
    return nullptr;
}

napi_value connect_broker(napi_env env,napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    //napi_valuetype valuetype;
    //napi_typeof(env, args[0], &valuetype);

    char broker[128],client_id[32];
    size_t result1;
    napi_get_value_string_utf8(env,args[0],broker,sizeof(broker),&result1);
    size_t result2;
    napi_get_value_string_utf8(env,args[1],client_id,sizeof(client_id),&result2);
    
    mqtt_context = std::make_unique<ahohc::mqtt::Context>(broker,client_id);
    
    return nullptr;
}