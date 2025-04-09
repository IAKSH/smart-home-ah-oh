#include "mqtt_ops.h"
#include "wifi_connect.h"
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "hi_time.h"
#include "cJSON.h"
#include "meta.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "iot_mqtt.h"

#define MQTT_CMD_TIMEOUT_MS 2000
#define MQTT_KEEP_ALIVE_MS 2000
#define MQTT_DELAY_2S 200
#define MQTT_DELAY_500_MS 50
#define MQTT_VERSION 3
#define MQTT_QOS 2
#define MQTT_TASK_STACK_SIZE (1024 * 10)
#define HTTP_BUF_SIZE 1024

static unsigned char sendBuf[1000];
static unsigned char readBuf[1000];

static Network network;  // 用于MQTT连接
static MQTTClient client;

char* generate_meta_json(void) {
    cJSON* meta_json = cJSON_CreateObject();
    cJSON* meta = cJSON_CreateObject();

    // type数组
    cJSON* type_array = cJSON_CreateArray();
    int i = 0;
    while (META.types[i] != NULL) {
        cJSON_AddItemToArray(type_array, cJSON_CreateString(META.types[i]));
        i++;
    }
    cJSON_AddItemToObject(meta, "type", type_array);
    cJSON_AddStringToObject(meta, "desc", META.desc);
    cJSON_AddNumberToObject(meta, "heartbeat_interval", META.heartbeat_interval);
    cJSON_AddStringToObject(meta, "attrib_schema", META.attrib_schema);

    // attrib数组
    cJSON* attrib_array = cJSON_CreateArray();
    for (i = 0; i < (sizeof(ATTRIB) / sizeof(Attrib)); i++) {
        cJSON* attrib_item = cJSON_CreateObject();
        cJSON_AddStringToObject(attrib_item, "topic", ATTRIB[i].topic);
        cJSON_AddStringToObject(attrib_item, "type", ATTRIB[i].type);
        cJSON_AddStringToObject(attrib_item, "desc", ATTRIB[i].desc);
        cJSON_AddStringToObject(attrib_item, "rw", ATTRIB[i].rw);
        cJSON_AddItemToArray(attrib_array, attrib_item);
    }
    cJSON_AddItemToObject(meta, "attrib", attrib_array);

    // 整体JSON：包含 device_id 和 meta
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", DEVICE_ID);
    cJSON_AddItemToObject(root, "meta", meta);

    char* json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

int http_upload_meta(const char* http_ip, uint16_t http_port) {
    int sockfd;
    struct sockaddr_in servAddr;
    char request[HTTP_BUF_SIZE * 2] = {0};
    char response[HTTP_BUF_SIZE] = {0};

    char* body = generate_meta_json();
    if (!body) {
        printf("[HTTP] Failed to generate meta JSON.\n");
        return -1;
    }
    int body_len = strlen(body);

    // 构造HTTP POST请求字符串（注意：换行符必须为"\r\n"）
    snprintf(request, sizeof(request),
             "POST /api/device HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s",
             http_ip, body_len, body);
    free(body);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("[HTTP] Socket creation failed: %s\n", strerror(errno));
        return -1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(http_port);
    if (inet_pton(AF_INET, http_ip, &servAddr.sin_addr) <= 0) {
        printf("[HTTP] Invalid server IP address format.\n");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        printf("[HTTP] Connect failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    if (send(sockfd, request, strlen(request), 0) < 0) {
        printf("[HTTP] Send failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    int n = recv(sockfd, response, sizeof(response) - 1, 0);
    if (n < 0) {
        printf("[HTTP] Receive failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    response[n] = '\0';
    close(sockfd);

    if (strstr(response, "200") != NULL || strstr(response, "201") != NULL) {
        printf("[HTTP] Meta uploaded successfully.\n");
        return 0;
    } else {
        printf("[HTTP] Meta upload failed, response: %s\n", response);
        return -1;
    }
}

void mqtt_init(const char* broker_ip, uint16_t broker_port) {
    WifiConnect("Nyatwork", "HelloNavi");
    printf("[MQTT] startup\n");
    int rc;

    NetworkInit(&network);
    printf("[MQTT] network connecting\n");

    NetworkConnect(&network, broker_ip, broker_port);
    printf("[MQTT] client initializing\n");
    MQTTClientInit(&client, &network, MQTT_CMD_TIMEOUT_MS, sendBuf, sizeof(sendBuf), readBuf, sizeof(readBuf));

    MQTTString clientId = MQTTString_initializer;
    clientId.cstring = "im_not_bearpi";

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.clientID = clientId;
    data.willFlag = 0;
    data.MQTTVersion = MQTT_VERSION;
    data.keepAliveInterval = MQTT_KEEP_ALIVE_MS;
    data.cleansession = 1;

    printf("[MQTT] connecting to broker at %s:%d\n", broker_ip, broker_port);
    rc = MQTTConnect(&client, &data);
    if (rc != 0) {
        printf("[MQTT] failed to connect to broker at \"%s:%d\", return code is: %d\n", broker_ip, broker_port, rc);
        NetworkDisconnect(&network);
        MQTTDisconnect(&client);
        osDelay(MQTT_DELAY_2S);
    }
}

static void mqtt_subscribe(const char* topic, void (*callback)(MessageData*)) {
    int rc = MQTTSubscribe(&client, topic, MQTT_QOS, callback);
    if (rc != 0) {
        printf("[MQTT] failed to subscribe topic %s, rc = %d\n", topic, rc);
    } else {
        printf("[MQTT] subscribed topic %s\n", topic);
    }
}

void mqtt_subscribe_all_rw_attrib(const Attrib* const attribs, int n, void (*callback)(MessageData*)) {
    char buffer[64];
    for (int i = 0; i < n; i++) {
        if (strcmp(attribs[i].rw, "rw") == 0) {
            snprintf(buffer, sizeof(buffer), "/device/%s/attrib%s", DEVICE_ID, attribs[i].topic);
            mqtt_subscribe(buffer, callback);
        }
    }
}

void publish_topic(const char* topic, const char* payload) {
    MQTTMessage message;
    message.qos = MQTT_QOS;
    message.retained = 0;
    message.payload = (void*)payload;
    message.payloadlen = strlen(payload);
    int rc = MQTTPublish(&client, topic, &message);
    if (rc != 0) {
        printf("[MQTT] failed to publish to topic \"%s\", return code %d\n", topic, rc);
        NetworkDisconnect(&network);
        MQTTDisconnect(&client);
    }
}

void publish_heartbeat(void) {
    char buffer[64];
    char url_base[64];
    snprintf(url_base, sizeof(url_base), "/device/%s/heartbeat", DEVICE_ID);
    snprintf(buffer, sizeof(buffer), "{\"timestamp\":%ld,\"status\":\"online\"}", hi_get_tick());
    publish_topic(url_base, buffer);
}

// TODO: abandoned
void publish_attrib(cJSON* json) {
    char* payload = cJSON_PrintUnformatted(json);
    char url_base[64];
    snprintf(url_base, sizeof(url_base), "/device/%s/attrib", DEVICE_ID);
    publish_topic(url_base, payload);
    free(payload);
}

double combine_ints_to_Double(int int_part, int fraction) {
    int fraction_digits = 0;
    int temp = fraction;
    if (temp == 0) {
        fraction_digits = 1;
    }
    else {
        if (temp < 0) temp = -temp;
        while (temp > 0) {
            fraction_digits++;
            temp /= 10;
        }
    }
    double denominator = pow(10, fraction_digits);
    double result = int_part + ((double)fraction) / denominator;
    return result;
}
