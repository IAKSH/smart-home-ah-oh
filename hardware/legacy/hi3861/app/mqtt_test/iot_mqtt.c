#include <ohos_init.h>
#include <cmsis_os2.h>
#include <stdio.h>
#include "wifi_connect.h"
#include "iot_mqtt.h"
#include "mqtt_ops.h"

static void on_msg_arrived_callback(MessageData* msg) {
    printf("onMessageArrivedCallback: %s\n",msg->message->payload);
}

static void uart_read_line(char *buffer, size_t max_length) {
    int i = 0;
    char ch;
    while (i < max_length - 1) {
        ch = getchar();  // 使用getchar模拟接收字符
        if (ch == '\n') break; // 遇到换行符结束
        buffer[i++] = ch;
    }
    buffer[i] = '\0'; // 添加字符串结束符

    // 去除字符串末尾的多余空格或换行符或回车符
    size_t length = strlen(buffer);
    if (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r')) {
        buffer[length - 1] = '\0';
    }
}

static void set_wifi(void) {
    char ssid[32], passwd[32];
    printf("enter ssid:\n");
    uart_read_line(ssid, sizeof(ssid));
    printf("enter passwd:\n");
    uart_read_line(passwd, sizeof(passwd));
    printf("[wifi] connecting to ssid: %s, passwd: %s\n", ssid, passwd);

    for (int i = 0; i < strlen(ssid); i++) {
        printf("[DEBUG] ssid[%d] = '%c' (ASCII: %d)\n", i, ssid[i], ssid[i]);
    }

    WifiConnect(ssid, passwd);
}

static void mqtt_app_task(void)
{
    char discovered_http_ip[32] = {0};
    uint16_t discovered_http_port = 0;
    char discovered_mqtt_ip[32] = {0};
    uint16_t discovered_mqtt_port = 0;

    // 0. 网络初始化
    set_wifi();

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
    printf("[MQTT] connecting to broker at %s:%d\n",discovered_mqtt_ip,discovered_mqtt_port);
    mqtt_init(discovered_mqtt_ip, discovered_mqtt_port);

    // 4. 发布所有attrib，到单独的topic
    // TODO
    
    // 5. 订阅自身所有可w属性
    //mqtt_subscribe(ATTRIB,onMessageArrivedCallback);
    mqtt_subscribe_all_rw_attrib(ATTRIB,sizeof(ATTRIB) / sizeof(Attrib),on_msg_arrived_callback);

    // 循环发布心跳和attrib数据
    int count = 0;
    while(++count) {
        publish_heartbeat();
        
        if(count % 10 == 0) {
            // TODO: 单独更新每个attrib到单独的topic
            cJSON* json = cJSON_CreateObject();
            //extern unsigned int dht11_data[4];
            //extern osMutexId_t dht11_mutex;
            //osMutexAcquire(dht11_mutex, osWaitForever);
            //cJSON_AddNumberToObject(json, "temperature", combine_ints_to_Double(dht11_data[2], dht11_data[3]));
            //cJSON_AddNumberToObject(json, "humidity", combine_ints_to_Double(dht11_data[0], dht11_data[1]));
            cJSON_AddNumberToObject(json, "temperature", combine_ints_to_Double(114, 514));
            cJSON_AddNumberToObject(json, "humidity", combine_ints_to_Double(1919, 810));
            //osMutexRelease(dht11_mutex);
            publish_attrib(json);
            cJSON_Delete(json);
        }

        osDelay(50);
    }
    
    // 发布遗嘱（will）消息
    const char* will = "{\"status\":\"offline\",\"will\":\"i don't wanna to work anymore\"}";
    char url_base[32];
    snprintf(url_base,sizeof(url_base),"/device/%s/will",DEVICE_ID);
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