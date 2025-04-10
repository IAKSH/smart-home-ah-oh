//
// Created on 2025/4/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HMOS_CPP_TEST_LOG_H
#define HMOS_CPP_TEST_LOG_H

#include "hilog/log.h"

#define AHOHC_LOG(level, tag, fmt, ...) \
    do { \
        if (*("" #__VA_ARGS__)) { \
            OH_LOG_Print(LOG_APP, level, 0, tag, fmt, ##__VA_ARGS__); \
        } else { \
            OH_LOG_Print(LOG_APP, level, 0, tag, fmt); \
        } \
    } while (0)

#endif //HMOS_CPP_TEST_LOG_H
