#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"

#include "MQTTClient.h"
#include "wifi_connect.h"

#define MQTT_SERVERIP CONFIG_MQTT_SERVER_IP
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

void messageArrived(MessageData *data)
{
    printf("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len,
        data->topicName->lenstring.data, data->message->payloadlen, data->message->payload);
}

static void MQTTDemoTask(void)
{
    WifiConnect(CONFIG_AP_SSID, CONFIG_AP_PASSWD);
    printf("Starting ...\n");
    int rc, count = 0;
    MQTTClient client;

    NetworkInit(&network);
    printf("NetworkConnect  ...\n");

    NetworkConnect(&network, MQTT_SERVERIP, MQTT_SERVERPORT);
    printf("MQTTClientInit  ...\n");
    MQTTClientInit(&client, &network, MQTT_CMD_TIMEOUT_MS, sendBuf, sizeof(sendBuf), readBuf, sizeof(readBuf));

    MQTTString clientId = MQTTString_initializer;
    clientId.cstring = "im_not_bearpi";

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.clientID = clientId;
    data.willFlag = 0;
    data.MQTTVersion = MQTT_VERSION;
    data.keepAliveInterval = MQTT_KEEP_ALIVE_MS;
    data.cleansession = 1;

    printf("MQTTConnect  ...\n");
    rc = MQTTConnect(&client, &data);
    if (rc != 0) {
        printf("MQTTConnect: %d\n", rc);
        NetworkDisconnect(&network);
        MQTTDisconnect(&client);
        osDelay(MQTT_DELAY_2S);
    }

    printf("MQTTSubscribe  ...\n");
    //rc = MQTTSubscribe(&client, "substopic", MQTT_QOS, messageArrived);
    rc = MQTTSubscribe(&client, "pubtopic", MQTT_QOS, messageArrived);
    if (rc != 0) {
        printf("MQTTSubscribe: %d\n", rc);
        osDelay(MQTT_DELAY_2S);
    }
    while (++count) {
        MQTTMessage message;
        char payload[30];

        message.qos = MQTT_QOS;
        message.retained = 0;
        message.payload = payload;
        (void)sprintf_s(payload, sizeof(payload), "message number %d", count);
        message.payloadlen = strlen(payload);

        if ((rc = MQTTPublish(&client, "pubtopic", &message)) != 0) {
            printf("Return code from MQTT publish is %d\n", rc);
            NetworkDisconnect(&network);
            MQTTDisconnect(&client);
        }
        else {
            printf("MQTT publish ok\n");
        }
        osDelay(MQTT_DELAY_500_MS);
    }
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
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)MQTTDemoTask, NULL, &attr) == NULL) {
        printf("[MQTT_Demo] Failed to create MQTTDemoTask!\n");
    }
}

APP_FEATURE_INIT(MQTTDemo);