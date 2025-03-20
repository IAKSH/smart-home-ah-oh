#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "hi_time.h" 
#include "iot_pwm.h"

// IR 发射及协议定义（与之前代码保持一致）
#define IR_PWM_CHANNEL    0
#define IR_PWM_FREQ       38000  // 38 kHz 载波
#define IR_PWM_DUTY       50

#define NEC_HDR_PULSE     9000
#define NEC_HDR_SPACE     4500
#define NEC_BIT_PULSE     560
#define NEC_BIT_0_SPACE   560
#define NEC_BIT_1_SPACE   1690
#define NEC_END_PULSE     560

#define IR_RX_PIN         HI_IO_NAME_GPIO_12
#define IR_RX_PIN_FUNC    HI_IO_FUNC_GPIO_12_GPIO
#define IR_TX_PIN         HI_IO_NAME_GPIO_9
#define IR_TX_PIN_FUNC    HI_IO_FUNC_GPIO_9_PWM0_OUT

#define MAX_IR_EDGES      100
#define END_GAP_THRESHOLD 10000  // 10ms

static uint32_t nec_decoded_data = 0;

// 用于边沿捕获的全局变量
static volatile int rx_index = 0;
static volatile uint32_t rx_durations[MAX_IR_EDGES] = {0};
static volatile uint32_t last_edge_time = 0;

// 由于SDK只支持单一极性，我们需要手动切换，目前设为全局变量
static IotGpioIntPolarity current_polarity = IOT_GPIO_EDGE_FALL_LEVEL_LOW;

// 辅助函数：获取当前时间（单位：微秒），依赖平台提供的 hi_time_get_us()
static inline uint32_t get_current_time_us(void) {
    return hi_get_us();
}

// IR 接收中断处理函数
static void IR_Rx_IRQHandler(void *arg) {
    (void)arg;
    uint32_t now = get_current_time_us();

    // 记录当前中断所对应的时间差
    if (rx_index == 0) {
        // 首次触发，只记录起始时间
        last_edge_time = now;
        rx_index = 1;
    } else {
        uint32_t interval = now - last_edge_time;
        last_edge_time = now;
        if (rx_index - 1 < MAX_IR_EDGES) {
            rx_durations[rx_index - 1] = interval;
            rx_index++;
        }
    }

    // 切换中断触发极性。当前检测的是下降沿，那么下次我们检测上升沿，反之亦然。
    if (current_polarity == IOT_GPIO_EDGE_FALL_LEVEL_LOW) {
        IoTGpioSetIsrMode(IR_RX_PIN, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_RISE_LEVEL_HIGH);
        current_polarity = IOT_GPIO_EDGE_RISE_LEVEL_HIGH;
    } else {
        IoTGpioSetIsrMode(IR_RX_PIN, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW);
        current_polarity = IOT_GPIO_EDGE_FALL_LEVEL_LOW;
    }
}

// 初始化红外接收功能：配置GPIO并注册中断
static void ir_receiver_init(void) {
    IoTGpioInit(IR_RX_PIN);
    // 设置引脚为输入模式
    IoTGpioSetDir(IR_RX_PIN, IOT_GPIO_DIR_IN);
    hi_io_set_func(IR_RX_PIN, IR_RX_PIN_FUNC);
    // 注册中断函数
    IoTGpioRegisterIsrFunc(IR_RX_PIN, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, IR_Rx_IRQHandler, NULL);
}

// 以下是 NEC 协议解析及其他代码（和之前类似）
// 根据捕获的时间间隔数据解析 NEC 协议（包含头部、bit数据判断等）
static uint32_t decode_nec_command(uint32_t *durations, int count) {
    if (count < (2 + 32 * 2)) {
        return 0xFFFFFFFF;  // 数据不足，解析失败
    }
    if (abs((int)durations[0] - NEC_HDR_PULSE) > (int)(NEC_HDR_PULSE * 0.2) ||
        abs((int)durations[1] - NEC_HDR_SPACE) > (int)(NEC_HDR_SPACE * 0.2)) {
         return 0xFFFFFFFF;
    }
    uint32_t command = 0;
    for (int i = 0; i < 32; i++) {
        int pulse_idx = 2 + i * 2;
        int space_idx = pulse_idx + 1;
        if (abs((int)durations[pulse_idx] - NEC_BIT_PULSE) > (int)(NEC_BIT_PULSE * 0.2))
            return 0xFFFFFFFF;
        uint32_t space_duration = durations[space_idx];
        if (abs((int)space_duration - NEC_BIT_0_SPACE) < (int)(NEC_BIT_0_SPACE * 0.2)) {
            // bit为0
        } else if (abs((int)space_duration - NEC_BIT_1_SPACE) < (int)(NEC_BIT_1_SPACE * 0.2)) {
            command |= (1 << i);
        } else {
            return 0xFFFFFFFF;
        }
    }
    return command;
}

static IotGpioValue io7_state = 0;

static void ir_rx_task(void *arg) {
    (void)arg;
    ir_receiver_init();

    while (1) {
        osDelay(100);  // 每隔100ms检查一次是否完成数据采集
        uint32_t now = get_current_time_us();
        if (rx_index > 1 && (now - last_edge_time) > END_GAP_THRESHOLD) {
            uint32_t durations_copy[MAX_IR_EDGES] = {0};
            int count = rx_index - 1;
            for (int i = 0; i < count && i < MAX_IR_EDGES; i++) {
                durations_copy[i] = rx_durations[i];
            }
            rx_index = 0;  // 重置采集计数

            uint32_t cmd = decode_nec_command(durations_copy, count);
            if (cmd != 0xFFFFFFFF) {
                nec_decoded_data = cmd;
                printf("Decoded NEC command: 0x%08X\n", cmd);
                // for test
                printf("io7_state = %d\n",io7_state);
                IoTGpioSetOutputVal(HI_IO_NAME_GPIO_7,io7_state);
                io7_state = io7_state == 0 ? 1 : 0;
            } else {
                printf("Failed to decode NEC signal. Captured durations count: %d\n", count);
            }
        }
    }
}

// 红外发射部分代码（与之前一致）
static void ir_pwm_init(void) {
    IoTGpioInit(IR_TX_PIN);
    hi_io_set_func(IR_TX_PIN, IR_TX_PIN_FUNC);
    IoTPwmInit(IR_PWM_CHANNEL);
}

static void ir_pwm_enable(void) {
    IoTPwmStart(IR_PWM_CHANNEL, IR_PWM_DUTY, IR_PWM_FREQ);
}

static void ir_pwm_disable(void) {
    IoTPwmStop(IR_PWM_CHANNEL);
}

static void nec_send_header(void) {
    ir_pwm_enable();
    hi_udelay(NEC_HDR_PULSE);
    ir_pwm_disable();
    hi_udelay(NEC_HDR_SPACE);
}

static void nec_send_bit(uint8_t bit) {
    ir_pwm_enable();
    hi_udelay(NEC_BIT_PULSE);
    ir_pwm_disable();
    if (bit) {
        hi_udelay(NEC_BIT_1_SPACE);
    } else {
        hi_udelay(NEC_BIT_0_SPACE);
    }
}

static void nec_send_command(uint32_t command) {
    int i;
    nec_send_header();
    for (i = 0; i < 32; i++) {
        uint8_t bit = (command >> i) & 0x1;
        nec_send_bit(bit);
    }
    // 结尾载波脉冲（可选）
    ir_pwm_enable();
    hi_udelay(NEC_END_PULSE);
    ir_pwm_disable();
}

static void ir_tx_task(void *arg) {
    (void)arg;
    ir_pwm_init();
    while (1) {
        nec_send_command(0x00FF00FF);
        printf("NEC command sent: 0x%08X\n", 0x00FF00FF);
        osDelay(250);
    }
}

static void ir_app_entry(void) {
    // for test
    IoTGpioInit(HI_IO_NAME_GPIO_7);
    hi_io_set_func(HI_IO_NAME_GPIO_7,HI_IO_FUNC_GPIO_7_GPIO);
    IoTGpioSetDir(HI_IO_NAME_GPIO_7,IOT_GPIO_DIR_OUT);

    osThreadAttr_t tx_attr = {0};
    tx_attr.name = "ir_tx_task";
    tx_attr.stack_size = 1024;
    tx_attr.priority = osPriorityNormal2;
    if (osThreadNew(ir_tx_task, NULL, &tx_attr) == NULL) {
        printf("Failed to create IR transmitter task.\n");
    }
  
    osThreadAttr_t rx_attr = {0};
    rx_attr.name = "ir_rx_task";
    rx_attr.stack_size = 1024;
    rx_attr.priority = osPriorityNormal2;
    if (osThreadNew(ir_rx_task, NULL, &rx_attr) == NULL) {
        printf("Failed to create IR receiver task.\n");
    }
}

SYS_RUN(ir_app_entry);
