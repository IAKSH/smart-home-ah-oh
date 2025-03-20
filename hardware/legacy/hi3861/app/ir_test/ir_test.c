#include <stdio.h>
#include <stdlib.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "hi_time.h" 
#include "iot_pwm.h"

// IR 发射 —— PWM 输出配置
#define IR_PWM_CHANNEL    0
#define IR_PWM_FREQ       38000  // 38 kHz 载波
#define IR_PWM_DUTY       50     // 50% 占空比

// NEC 协议时序（单位：微秒）
#define NEC_HDR_PULSE     9000
#define NEC_HDR_SPACE     4500
#define NEC_BIT_PULSE     560
#define NEC_BIT_0_SPACE   560
#define NEC_BIT_1_SPACE   1690
#define NEC_END_PULSE     560

// IR 接收 —— 假定 IR 接收模块由 GPIO 提供数据（请根据实际情况选用正确引脚）
#define IR_RX_PIN         HI_IO_NAME_GPIO_12
#define IR_RX_PIN_FUNC HI_IO_FUNC_GPIO_12_GPIO
#define IR_TX_PIN         HI_IO_NAME_GPIO_9
//#define IR_TX_PIN HI_IO_NAME_GPIO_7
#define IR_TX_PIN_FUNC HI_IO_FUNC_GPIO_9_PWM0_OUT
//#define IR_TX_PIN_FUNC HI_IO_FUNC_GPIO_7_GPIO
#define MAX_IR_EDGES      100

static uint32_t nec_decoded_data = 0;

static void ir_pwm_init(void) {
    // 初始化 PWM 通道，用于生成38KHz载波
    IoTGpioInit(IR_TX_PIN);
    hi_io_set_func(IR_TX_PIN,IR_TX_PIN_FUNC);
    IoTPwmInit(IR_PWM_CHANNEL);
}

static void ir_pwm_enable(void) {
    IoTPwmStart(IR_PWM_CHANNEL,IR_PWM_DUTY,IR_PWM_FREQ);
    //IoTGpioSetOutputVal(IR_TX_PIN,IOT_GPIO_VALUE1);
}

static void ir_pwm_disable(void) {
    IoTPwmStop(IR_PWM_CHANNEL);
    //IoTGpioSetOutputVal(IR_TX_PIN,IOT_GPIO_VALUE0);
}

// 发送 NEC 协议头部：9 ms 载波脉冲 + 4.5 ms 空白
static void nec_send_header(void) {
    ir_pwm_enable();
    hi_udelay(NEC_HDR_PULSE);
    ir_pwm_disable();
    hi_udelay(NEC_HDR_SPACE);
}

// 发送一位 NEC 数据
// 每位先发送 560 µs 的载波脉冲
// 随后空白 560 µs（逻辑 0）或 1690 µs（逻辑 1）
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

// 发送完整的 32 位 NEC 命令
// NEC 协议通常规定传输 8 位地址、8 位地址反码、8 位命令、8 位命令反码，
// 本例将 32 位数据按低位先发送
static void nec_send_command(uint32_t command) {
    int i;
    // 发送头部
    nec_send_header();
    // 发送 32 位数据（LSB 优先）
    for (i = 0; i < 32; i++) {
        uint8_t bit = (command >> i) & 0x1;
        nec_send_bit(bit);
    }
    // 最后发送一个结尾载波脉冲（可选）
    ir_pwm_enable();
    hi_udelay(NEC_END_PULSE);
    ir_pwm_disable();
}

static void ir_tx_task(void *arg) {
    (void)arg;
    // 初始化 PWM 用于 IR 输出
    ir_pwm_init();

    while (1) {
        // 示例：发送一个 NEC 命令 0x00FF00FF（通常表示地址与命令及其反码）
        nec_send_command(0x00FF00FF);
        printf("NEC command sent: 0x%08X\n", 0x00FF00FF);
        osDelay(250);
    }
}

static osSemaphoreId_t ir_rx_semaphore;

static void IR_Rx_IRQHandler(void *arg) {
    (void)arg;
    osSemaphoreRelease(ir_rx_semaphore);
}

static void ir_receiver_init(void) {
    ir_rx_semaphore = osSemaphoreNew(1,0,NULL);
    // 初始化 IR 接收引脚
    IoTGpioInit(IR_RX_PIN);
    hi_io_set_func(IR_RX_PIN,IR_RX_PIN_FUNC);
    IoTGpioSetDir(IR_RX_PIN, IOT_GPIO_DIR_IN);
    hi_io_set_pull(IR_RX_PIN, HI_IO_PULL_UP);
    // 注册 GPIO 中断
    IoTGpioRegisterIsrFunc(IR_RX_PIN, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, IR_Rx_IRQHandler,NULL);
}

static void ir_rx_task(void *arg) {
    (void)arg;
    IotGpioValue val;
    uint32_t us_time;
    uint8_t i,j,data;
    ir_receiver_init();
    while (1) {
        osSemaphoreAcquire(ir_rx_semaphore,osWaitForever);

        // NEC首部高电平 (接收器输出反向，下同)
        us_time = hi_get_us();
        while(IoTGpioGetInputVal(IR_RX_PIN,&val),val == 0);
        us_time = hi_get_us() - us_time;
        // 标准时间: 9000us
        if(us_time < 7000 || us_time > 10000) {
            printf("[decode] NEC head 1 invalid, us_time=%u\n",us_time);
            continue;
        }

        // NEC首部低电平
        us_time = hi_get_us();
        while(IoTGpioGetInputVal(IR_RX_PIN,&val),val == 1);
        us_time = hi_get_us() - us_time;
        // 标准时间: 4500us
        if(us_time < 3000 || us_time > 5500) {
            printf("[decode] NEC head 0 invalid, us_time=%u\n",us_time);
            continue;
        }

        for(i = 0;i < 4;i++) {
            for(j = 0;j < 8;j++) {
                us_time = hi_get_us();
                while(IoTGpioGetInputVal(IR_RX_PIN,&val),val == 0);
                us_time = hi_get_us() - us_time;
                // 标准时间: 560us
                if(us_time < 400 || us_time > 700) {
                    printf("[decode] NEC data 1 invalid, us_time=%u\n",us_time);
                    continue;
                }
                    
                
                us_time = hi_get_us();
                while(IoTGpioGetInputVal(IR_RX_PIN,&val),val == 1);
                us_time = hi_get_us() - us_time;
                // NEC编码1, 1680us
                if(us_time > 1400 && us_time < 1800) {
                    data >>= 1;
                    data |= 0x80;
                }
                // NEC编码0, 560us
                else if(us_time > 400 && us_time < 700) {
                    data >>= 1;
                }
                else {
                    printf("[decode] NEC data 0 invalid, us_time=%u\n",us_time);
                    continue;
                }
            }
            ((uint8_t*)nec_decoded_data)[i] = data;
        }
        printf("[decode] decoded: 0x%X\n",nec_decoded_data);
    }
}

static void ir_app_entry(void) {
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
