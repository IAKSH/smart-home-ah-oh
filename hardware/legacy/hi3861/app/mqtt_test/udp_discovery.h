#ifndef UDP_DISCOVERY_H
#define UDP_DISCOVERY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 通过UDP广播查询服务器地址，回复应为JSON格式，包含：
 *   "server_ip", "server_port", "mqtt_broker_ip", "mqtt_broker_port"
 *
 * 参数：
 *   http_ip: 存放HTTP服务器IP地址的缓冲区
 *   http_ip_size: 缓冲区大小
 *   http_port: 返回HTTP服务器端口
 *   mqtt_ip: 存放MQTT Broker IP地址的缓冲区
 *   mqtt_ip_size: 缓冲区大小
 *   mqtt_port: 返回MQTT Broker端口
 *
 * 返回值：成功返回0，否则返回 -1。
 */
int discover_server(char* http_ip, int http_ip_size, uint16_t* http_port,
                    char* mqtt_ip, int mqtt_ip_size, uint16_t* mqtt_port);

#ifdef __cplusplus
}
#endif

#endif /* UDP_DISCOVERY_H */
