#include "udp_discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "cJSON.h"

#define UDP_DISCOVERY_PORT 8888
#define UDP_DISCOVERY_MSG "AHOH_DISCOVER_SERVER"
#define UDP_RCV_TIMEOUT_MS 5000
#define HTTP_BUF_SIZE 1024
#define BROADCAST_IP "255.255.255.255"

int discover_server(char* http_ip, int http_ip_size, uint16_t* http_port,
                    char* mqtt_ip, int mqtt_ip_size, uint16_t* mqtt_port) {
    int sockfd;
    struct sockaddr_in bcastAddr, recvAddr;
    socklen_t addr_len = sizeof(recvAddr);
    char buffer[HTTP_BUF_SIZE] = {0};

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("[UDP] Socket creation failed: %s\n", strerror(errno));
        return -1;
    }

    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        printf("[UDP] Failed to set broadcast option: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    memset(&bcastAddr, 0, sizeof(bcastAddr));
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_port = htons(UDP_DISCOVERY_PORT);
    bcastAddr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    int sent = sendto(sockfd, UDP_DISCOVERY_MSG, strlen(UDP_DISCOVERY_MSG), 0,
                      (struct sockaddr*)&bcastAddr, sizeof(bcastAddr));
    if (sent < 0) {
        printf("[UDP] Failed to send broadcast message: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    printf("[UDP] Broadcast message sent. Waiting for response...\n");

    struct timeval timeout;
    timeout.tv_sec = UDP_RCV_TIMEOUT_MS / 1000;
    timeout.tv_usec = (UDP_RCV_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr*)&recvAddr, &addr_len);
    if (n < 0) {
        printf("[UDP] Failed to receive response or timed out.\n");
        close(sockfd);
        return -1;
    }
    buffer[n] = '\0';
    close(sockfd);

    cJSON* json = cJSON_Parse(buffer);
    if (!json) {
        printf("[UDP] Failed to parse JSON response.\n");
        return -1;
    }
    cJSON* j_http_ip = cJSON_GetObjectItem(json, "server_ip");
    cJSON* j_http_port = cJSON_GetObjectItem(json, "server_port");
    cJSON* j_mqtt_ip = cJSON_GetObjectItem(json, "mqtt_broker_ip");
    cJSON* j_mqtt_port = cJSON_GetObjectItem(json, "mqtt_broker_port");

    if (!j_http_ip || !j_http_port || !j_mqtt_ip || !j_mqtt_port) {
        printf("[UDP] JSON response missing required fields.\n");
        cJSON_Delete(json);
        return -1;
    }
    strncpy(http_ip, j_http_ip->valuestring, http_ip_size - 1);
    *http_port = (uint16_t)atoi(j_http_port->valuestring);
    strncpy(mqtt_ip, j_mqtt_ip->valuestring, mqtt_ip_size - 1);
    *mqtt_port = (uint16_t)atoi(j_mqtt_port->valuestring);
    cJSON_Delete(json);

    printf("[UDP] Discovered HTTP server: %s:%d, MQTT broker: %s:%d\n",
           http_ip, *http_port, mqtt_ip, *mqtt_port);
    return 0;
}
