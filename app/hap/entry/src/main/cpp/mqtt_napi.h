//
// Created on 2025/4/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HMOS_CPP_TEST_MQTT_NAPI_H
#define HMOS_CPP_TEST_MQTT_NAPI_H

#include <napi/native_api.h>
#include <memory>
#include "mqtt.h"

extern std::unique_ptr<ahohc::mqtt::Context> mqtt_context;
extern napi_threadsafe_function tsfn;

napi_value register_mqtt_callback(napi_env env, napi_callback_info info);
napi_value subscribe(napi_env env, napi_callback_info info);
napi_value publish(napi_env env, napi_callback_info info);
napi_value connect_broker(napi_env env,napi_callback_info info);

#endif //HMOS_CPP_TEST_MQTT_NAPI_H
