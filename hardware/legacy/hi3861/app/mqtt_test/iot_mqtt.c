#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "hi_time.h"

#include "MQTTClient.h"
#include "wifi_connect.h"

#include "cJSON.h"

//#define MQTT_SERVERIP
#define MQTT_SERVERPORT 1883
#define MQTT_CMD_TIMEOUT_MS 2000
#define MQTT_KEEP_ALIVE_MS 2000
#define MQTT_DELAY_2S 200
#define MQTT_DELAY_500_MS 50
#define MQTT_VERSION 3
#define MQTT_QOS 2
#define MQTT_TASK_STACK_SIZE (1024 * 10)

static unsigned char sendBuf[1000];
static unsigned char readBuf[1000];

Network network;

static void messageArrived(MessageData *data) {
    printf("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len,
        data->topicName->lenstring.data, data->message->payloadlen, data->message->payload);
}

static void subscribe_topic(MQTTClient* client,const char* topic) {
    int rc = MQTTSubscribe(client, topic, MQTT_QOS, messageArrived);
    if (rc != 0)
        printf("[mqtt] failed to subscribed topic \"%s\". return code is %d\n", topic, rc);
}

static void publish_topic(MQTTClient* client,const char* topic,MQTTMessage* message) {
    int rc;
    if ((rc = MQTTPublish(client, topic, message)) != 0) {
        printf("[mqtt] failed to publish to topic \"%s\", return code from MQTT publish is %d\n", topic, rc);
        NetworkDisconnect(&network);
        MQTTDisconnect(client);
    }
}

static MQTTClient client;

static void publish_common_topic(const char* topic,const char* payload) {
    MQTTMessage message;
    message.qos = MQTT_QOS;
    message.retained = 0;
    message.payload = payload;
    message.payloadlen = strlen(payload);
    publish_topic(&client,topic,&message);
}

static void mqtt_init(void) {
    WifiConnect(MQTT_AP_SSID, MQTT_AP_PASSWD);
    printf("[mqtt] startup\n");
    int rc;

    NetworkInit(&network);
    printf("[mqtt] network connecting\n");

    NetworkConnect(&network, MQTT_SERVERIP, MQTT_SERVERPORT);
    printf("[mqtt] client initializing\n");
    MQTTClientInit(&client, &network, MQTT_CMD_TIMEOUT_MS, sendBuf, sizeof(sendBuf), readBuf, sizeof(readBuf));

    MQTTString clientId = MQTTString_initializer;
    clientId.cstring = "im_not_bearpi";

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.clientID = clientId;
    data.willFlag = 0;
    data.MQTTVersion = MQTT_VERSION;
    data.keepAliveInterval = MQTT_KEEP_ALIVE_MS;
    data.cleansession = 1;

    printf("[mqtt] connecting\n");
    rc = MQTTConnect(&client, &data);
    if (rc != 0) {
        printf("[mqtt] failed to connect to broker at \"%s:%d\", return code is: %d\n", MQTT_SERVERIP,MQTT_SERVERPORT,rc);
        NetworkDisconnect(&network);
        MQTTDisconnect(&client);
        osDelay(MQTT_DELAY_2S);
    }
}

static void mqtt_subscribes(void) {
    printf("[mqtt] subscribing topics\n");
    subscribe_topic(&client,"pubtopic");
}

typedef struct {
    char* topic;
    char* type;
    char* desc;
    char* rw;
} Attrib;

typedef struct {
    char** types;
    char* desc;
    int heartbeat_interval;
    char* attrib_schema;
    Attrib* attrib;
} Meta;

static const Attrib ATTRIB[] = {
    {
        .topic = "/temperature",
        .type = "float",
        .desc = "摄氏温度传感器读数",
        .rw = "r"
    },
    {
        .topic = "/humidity",
        .type = "float",
        .desc = "湿度传感器读数",
        .rw = "r"
    },
    {
        .topic = "/alert",
        .type = "bool",
        .desc = "传感器报警状态",
        .rw = "rw"
    }
};

static const char *meta_types[] = {"thermometer", "hygrometer", NULL};

static const Meta META = {
    .types = meta_types,
    .desc = "Hi3861 Device for test",
    .heartbeat_interval = 10,
    .attrib_schema = "v1",
    .attrib = ATTRIB
};

static cJSON* generate_meta(void) {
    cJSON* type_array = cJSON_CreateArray();
    int i = 0;
    while(META.types[i] != NULL)
        cJSON_AddItemToArray(type_array,cJSON_CreateString(META.types[i++]));

    cJSON* meta_root = cJSON_CreateObject();
    cJSON_AddItemToObject(meta_root,"type",type_array);

    cJSON_AddStringToObject(meta_root,"desc",META.desc);
    cJSON_AddNumberToObject(meta_root,"heartbeat_interval",META.heartbeat_interval);
    cJSON_AddStringToObject(meta_root,"attrib_schema",META.attrib_schema);
    
    cJSON* attrib_array = cJSON_CreateArray();
    for(i = 0;i < sizeof(ATTRIB) / sizeof(Attrib);i++) {
        cJSON* attrib_item = cJSON_CreateObject();
        cJSON_AddStringToObject(attrib_item,"topic",META.attrib[i].topic);
        cJSON_AddStringToObject(attrib_item,"type",META.attrib[i].type);
        cJSON_AddStringToObject(attrib_item,"desc",META.attrib[i].desc);
        cJSON_AddStringToObject(attrib_item,"rw",META.attrib[i].rw);
        cJSON_AddItemToArray(attrib_array,attrib_item);
    }
    cJSON_AddItemToObject(meta_root,"attrib",attrib_array);
    return meta_root;
}

#define DEVICE_ID "hi3861_1"

static void publish_meta(void) {
    char* json_print;
    char url_base[32];
    cJSON* meta = generate_meta();
    json_print = cJSON_Print(meta);
    sprintf_s(url_base,sizeof(url_base),"/device/%s/meta",DEVICE_ID);
    publish_common_topic(url_base,json_print);
    cJSON_Delete(meta);
    free(json_print);
}

static void publish_heartbeat(void) {
    char buffer[32];
    char url_base[32];
    sprintf_s(buffer,sizeof(buffer),"{\"timestamp\":%ld,\"status\":\"%s\"}",hi_get_tick(),"online");
    sprintf_s(url_base,sizeof(url_base),"/device/%s/heartbeat",DEVICE_ID);
    publish_common_topic(url_base,buffer);
}

static void publish_will(void) {
    const char* will = "{\"status\":\"offline\",\"will\":\"i don't wanna to work anymore\"}";
    char url_base[32];
    sprintf_s(url_base,sizeof(url_base),"/device/%s/will",DEVICE_ID);
    publish_common_topic(url_base,will);
}

static void publish_attrib(cJSON* json) {
    char* payload = cJSON_Print(json);
    char url_base[32];
    sprintf_s(url_base,sizeof(url_base),"/device/%s/attrib",DEVICE_ID);
    publish_common_topic(url_base,payload);
    free(payload);
}

static double combine_ints_to_Double(int int_part,int fraction) {
    int fraction_digits = 0;
    int temp = fraction;
    
    // 如果 fraction 为 0，则数字位数设为 1，否则计算其位数
    if (temp == 0) {
        fraction_digits = 1;
    } else {
        // 若 fraction 为负数，此处可取其绝对值，以便正确计数
        if (temp < 0) {
            temp = -temp;
        }
        while (temp > 0) {
            fraction_digits++;
            temp /= 10;
        }
    }
    
    // 根据小数位数计算除数，例如小数位数为 3 时，除数为 1000
    double denominator = pow(10, fraction_digits);
    
    // 将 intPart 和转换后的 fraction 组合成 double 数值
    double result = int_part + ((double)fraction) / denominator;
    return result;
}

static void MQTTDemoTask(void) {
    mqtt_init();
    mqtt_subscribes();
    publish_meta();

    int count = 0;
    while (++count) {
        publish_heartbeat();

        cJSON* json = cJSON_CreateObject();
        extern unsigned int dht11_data[4];
        extern osMutexId_t dht11_mutex;
        osMutexAcquire(dht11_mutex, osWaitForever);
        cJSON_AddNumberToObject(json,"temperature",combine_ints_to_Double(dht11_data[2],dht11_data[3]));
        cJSON_AddNumberToObject(json,"humidity",combine_ints_to_Double(dht11_data[0],dht11_data[1]));
        osMutexRelease(dht11_mutex);
        publish_attrib(json);
        cJSON_Delete(json);

        osDelay(MQTT_DELAY_500_MS);
    }

    publish_will();
}
static void MQTTDemo(void)
{
    osThreadAttr_t attr;

    attr.name = "MQTTDemoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = MQTT_TASK_STACK_SIZE;
    attr.priority = osPriorityBelowNormal;

    if (osThreadNew((osThreadFunc_t)MQTTDemoTask, NULL, &attr) == NULL) {
        printf("[MQTT_Demo] Failed to create MQTTDemoTask!\n");
    }
}

APP_FEATURE_INIT(MQTTDemo);