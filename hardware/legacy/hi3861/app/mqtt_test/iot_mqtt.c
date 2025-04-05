#include <ohos_init.h>
#include <cmsis_os2.h>
#include "wifi_connect.h"
#include "iot_mqtt.h"
#include "mqtt_ops.h"

void mqtt_app_task(void)
{
    char discovered_http_ip[32] = {0};
    uint16_t discovered_http_port = 0;
    char discovered_mqtt_ip[32] = {0};
    uint16_t discovered_mqtt_port = 0;

    // 0. 网络初始化
    WifiConnect("Nyatwork","HelloNavi");

    // 1. 通过UDP广播查询服务器地址（重试直至成功），使用udp_discovery模块提供的接口应在主工程中调用此函数
    // 此处直接调用 discover_server (需在编译时链接 udp_discovery.c)
    while(discover_server(discovered_http_ip, sizeof(discovered_http_ip), &discovered_http_port,
                          discovered_mqtt_ip, sizeof(discovered_mqtt_ip), &discovered_mqtt_port) != 0) {
        printf("[DISCOVERY] Failed to discover server addresses, retrying in 3 seconds...\n");
        osDelay(300);
    }
    
    // 2. 通过HTTP API上传设备元数据（重试直至成功）
    while(http_upload_meta(discovered_http_ip, discovered_http_port) != 0) {
        printf("[HTTP] Metadata upload failed, retrying in 3 seconds...\n");
        osDelay(300);
    }
    
    // 3. 元数据上传成功后，开始MQTT通讯
    mqtt_init(discovered_mqtt_ip, discovered_mqtt_port);
    mqtt_subscribes();
    
    // 循环发布心跳和attrib数据
    int count = 0;
    while(++count) {
        publish_heartbeat();
        
        cJSON* json = cJSON_CreateObject();
        extern unsigned int dht11_data[4];
        extern osMutexId_t dht11_mutex;
        osMutexAcquire(dht11_mutex, osWaitForever);
        cJSON_AddNumberToObject(json, "temperature", combine_ints_to_Double(dht11_data[2], dht11_data[3]));
        cJSON_AddNumberToObject(json, "humidity", combine_ints_to_Double(dht11_data[0], dht11_data[1]));
        osMutexRelease(dht11_mutex);
        publish_attrib(json);
        cJSON_Delete(json);
        
        osDelay(50);
    }
    
    // 发布遗嘱（will）消息
    const char* will = "{\"status\":\"offline\",\"will\":\"i don't wanna to work anymore\"}";
    char url_base[32];
    sprintf(url_base, "/device/%s/will", DEVICE_ID);
    publish_topic(url_base, will);
}

static void mqtt_app_entry(void) {
    osThreadAttr_t attr;
    attr.name = "MQTTDemoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 10;
    attr.priority = osPriorityBelowNormal;

    if(osThreadNew((osThreadFunc_t)mqtt_app_task, NULL, &attr) == NULL) {
        printf("[MQTT_Demo] Failed to create MQTTDemoTask!\n");
    }
}

SYS_RUN(mqtt_app_entry);