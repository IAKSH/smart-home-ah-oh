#ifndef MQTT_OPS_H
#define MQTT_OPS_H

#include <stdint.h>
#include "MQTTClient.h"
#include "cJSON.h"
#include "meta.h"

#ifdef __cplusplus
extern "C" {
#endif

char* generate_meta_json(void);
int http_upload_meta(const char* http_ip, uint16_t http_port);
void mqtt_init(const char* broker_ip, uint16_t broker_port);
void publish_topic(const char* topic, const char* payload);
void publish_heartbeat(void);
void publish_attrib(cJSON* json);
double combine_ints_to_Double(int int_part, int fraction);
void mqtt_subscribe_all_rw_attrib(const Attrib* const attribs,int n,void(*callback)(MessageData*));

#ifdef __cplusplus
}
#endif

#endif /* MQTT_OPS_H */
