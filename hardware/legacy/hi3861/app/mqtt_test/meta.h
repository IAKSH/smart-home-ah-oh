#pragma once

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
    const Attrib* attrib;
} Meta;