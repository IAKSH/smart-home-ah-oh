#ifndef MQTT_OPS_H
#define MQTT_OPS_H

#include <stdint.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 生成设备元数据的JSON字符串，格式保持不变。
 * 返回值为动态分配的字符串，需要使用free()释放。
 */
char* generate_meta_json(void);

/**
 * 通过HTTP POST方式上传设备元数据。
 * 参数 http_ip 为HTTP服务器IP地址，http_port 为其端口。
 * 成功返回0，失败返回-1。
 */
int http_upload_meta(const char* http_ip, uint16_t http_port);

/**
 * 初始化MQTT连接，参数为MQTT Broker的IP地址和端口。
 */
void mqtt_init(const char* broker_ip, uint16_t broker_port);

/**
 * MQTT订阅（如有需要时添加订阅操作）。
 */
void mqtt_subscribes(void);

/**
 * 通过MQTT发送消息到指定主题。
 */
void publish_topic(const char* topic, const char* payload);

/**
 * 发布心跳消息。
 */
void publish_heartbeat(void);

/**
 * 发布attrib数据，通过给定的 cJSON 对象构造消息。
 */
void publish_attrib(cJSON* json);

/**
 * 将整数部分和小数部分组合成double数值。
 */
double combine_ints_to_Double(int int_part, int fraction);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_OPS_H */
