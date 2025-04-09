#ifndef UDP_DISCOVERY_H
#define UDP_DISCOVERY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int discover_server(char* http_ip, int http_ip_size, uint16_t* http_port,
                    char* mqtt_ip, int mqtt_ip_size, uint16_t* mqtt_port);

#ifdef __cplusplus
}
#endif

#endif /* UDP_DISCOVERY_H */
