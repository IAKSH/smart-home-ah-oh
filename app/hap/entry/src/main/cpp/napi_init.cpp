#include <napi/native_api.h>
#include <sstream>
#include <string>
#include <string_view>
#include "api.h"
#include "mqtt_napi.h"
#include "utils/log.h"

static const char* TAG = "napi_adaptor";

static napi_value napi_callback_test(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, nullptr, "Callback function missing");
        return nullptr;
    }

    napi_valuetype valuetype;
    napi_typeof(env, args[0], &valuetype);
    if (valuetype != napi_function) {
        napi_throw_error(env, nullptr, "Expected a function as the first argument");
        return nullptr;
    }

    // 创建一个 JavaScript 字符串
    napi_value js_string;
    constexpr std::string_view BASE_STR = "Hello from native code!";
    static int cnt = 0;
    std::stringstream ss;
    ss << BASE_STR << " cnt = " << cnt++;
    napi_create_string_utf8(env, ss.str().c_str(), NAPI_AUTO_LENGTH, &js_string);

    // 获取全局对象
    napi_value global;
    napi_get_global(env, &global);

    // 调用回调函数并传递参数
    napi_value result;
    napi_call_function(env, global, args[0], 1, &js_string, &result);

    return nullptr;
}

static napi_value Add(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    double value0;
    napi_get_value_double(env, args[0], &value0);

    double value1;
    napi_get_value_double(env, args[1], &value1);

    napi_value sum;
    napi_create_double(env, value0 + value1, &sum);

    return sum;
}

static napi_value connect_server(napi_env env,napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);
    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    char ip[16],port[6];
    size_t result;
    napi_get_value_string_utf8(env,args[0],ip,sizeof(ip),&result);
    napi_get_value_string_utf8(env,args[1],port,sizeof(port),&result);
    
    ahohc::api::connect_to_server(ip, port);
    
    return nullptr;
}

static napi_value fetch_devices(napi_env env,napi_callback_info info) {
    auto respond = ahohc::api::fetch_devices();
    napi_value ret;
    napi_create_string_utf8(env,respond.c_str(),respond.length(),&ret);
    return ret;
}

static napi_value fetch_device(napi_env env,napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype;
    napi_typeof(env, args[0], &valuetype);

    char device_id[32];
    size_t result;
    napi_get_value_string_utf8(env,args[0],device_id,sizeof(device_id),&result);
    
    auto respond = ahohc::api::fetch_device(device_id);
    
    napi_value ret;
    napi_create_string_utf8(env,respond.c_str(),respond.length(),&ret);
    return ret;
}

static napi_value delete_device(napi_env env,napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype;
    napi_typeof(env, args[0], &valuetype);

    char device_id[32];
    size_t result;
    napi_get_value_string_utf8(env,args[0],device_id,sizeof(device_id),&result);
    
    ahohc::api::delete_device(device_id);
    
    return nullptr;
}

static napi_value init_udp_context(napi_env env,napi_callback_info info) {
    ahohc::api::init_udp_context();
    return nullptr;
}

static napi_value discover_server(napi_env env,napi_callback_info info) {
    auto response = ahohc::api::discover_server();
    napi_value ret;
    napi_create_string_utf8(env,response.c_str(),response.length(),&ret);
    return ret;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"add", nullptr, Add, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"connectServer", nullptr, connect_server, nullptr, nullptr, nullptr, napi_default, nullptr },
        {"fetchDevices", nullptr, fetch_devices, nullptr, nullptr, nullptr, napi_default, nullptr },
        {"fetchDevice", nullptr, fetch_device, nullptr, nullptr, nullptr, napi_default, nullptr },
        {"deleteDevice", nullptr, delete_device, nullptr, nullptr, nullptr, napi_default, nullptr },
        {"initUdpContext", nullptr, init_udp_context, nullptr, nullptr, nullptr, napi_default, nullptr },
        {"discoverServer", nullptr, discover_server, nullptr, nullptr, nullptr, napi_default, nullptr },
        {"napiCallbackTest", nullptr, napi_callback_test, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "registerMQTTCallback", nullptr, register_mqtt_callback, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "subscribe",            nullptr, subscribe,            nullptr, nullptr, nullptr, napi_default, nullptr },
        { "publish",              nullptr, publish,              nullptr, nullptr, nullptr, napi_default, nullptr },
        { "connectBroker",        nullptr, connect_broker,       nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    
    // 初始化全局 MQTT context，使用默认的 broker 地址和 clientId
    // 例如：tcp://mqtt.eclipse.org:1883 作为 broker，"ArkTS_MQTT_Client" 为 clientId
    //g_mqtt_context = new ahohc::mqtt::Context("tcp://mqtt.eclipse.org:1883", "ArkTS_MQTT_Client");
    // seems not work, try to give a dedicated api
    
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
    napi_module_register(&demoModule);
}
