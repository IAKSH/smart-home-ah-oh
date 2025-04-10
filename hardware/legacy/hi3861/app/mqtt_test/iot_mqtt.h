#pragma once
#include <stdlib.h>
#include <cmsis_os2.h>
#include "meta.h"

#define DEVICE_ID "hi3861_1"

inline static const Attrib ATTRIB[] = {
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
    },
    {
        .topic = "/power_on",
        .type = "bool",
        .desc = "空调开关",
        .rw = "rw"
    }
};

inline static const char *meta_types[] = {"thermometer", "hygrometer", NULL};

inline static const Meta META = {
    .types = meta_types,
    .desc = "Hi3861 Device for test",
    .heartbeat_interval = 10,
    .attrib_schema = "v1",
    .attrib = ATTRIB
};

extern osEventFlagsId_t mqtt_event_flags;