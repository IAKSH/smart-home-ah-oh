#include "nec.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <hi_io.h>
#include <hi_time.h>
#include <iot_gpio.h>
#include <cmsis_os2.h>

#define TOLERANCE_PERCENT    20   // 允许±20%的误差

// 判断测量值是否在预期范围内
static inline bool within_tolerance(uint32_t measured, uint32_t expected) {
    uint32_t lower = expected * (100 - TOLERANCE_PERCENT) / 100;
    uint32_t upper = expected * (100 + TOLERANCE_PERCENT) / 100;
    return (measured >= lower) && (measured <= upper);
}

uint32_t nec_decoded;  // 在头文件中已声明extern

#define IR_RX_PIN         HI_IO_NAME_GPIO_12
#define IR_RX_PIN_FUNC    HI_IO_FUNC_GPIO_12_GPIO

static osSemaphoreId_t nec_rx_sem;

static void nec_rx_irq_handler(void *arg) {
    osSemaphoreRelease(nec_rx_sem);
}

static void nec_rx_init(void) {
    nec_rx_sem = osSemaphoreNew(1, 0, NULL);
    IoTGpioInit(IR_RX_PIN);
    IoTGpioSetDir(IR_RX_PIN, IOT_GPIO_DIR_IN);
    hi_io_set_func(IR_RX_PIN, IR_RX_PIN_FUNC);
    IoTGpioRegisterIsrFunc(IR_RX_PIN, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, nec_rx_irq_handler, NULL);
}

static IotGpioValue IoTGpioGetVal(int gpio_id) {
    static IotGpioValue val;
    IoTGpioGetInputVal(gpio_id,&val);
    return val;
}

/*
 * NEC 协议解码流程：
 * 1. 等待由中断唤醒，本次下降沿作为信号开始（Header Pulse开始）。
 * 2. 测量低电平持续时长，判断是否为约9000us的Header Pulse。
 * 3. 测量升高后保持的高电平（Header Space，约4500us）。
 * 4. 循环32次：
 *    a. 测量每个数据位的低电平脉宽（应为560us）。
 *    b. 紧接着测量高电平长度，根据其在560us或1690us附近确定是0还是1。（NEC数据一般低位先传输）
 * 5. 可选：测量结束脉冲（560us）。
 * 6. 将解码得到的32位命令存入全局变量nec_decoded
 */
static void nec_rx_decode_task(void* args) {
    nec_rx_init();
    
    while(1) {
        // 阻塞等待下降沿中断（Header脉冲开始）
        //IoTGpioRegisterIsrFunc(IR_RX_PIN, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, nec_rx_irq_handler, NULL);
        osSemaphoreAcquire(nec_rx_sem, osWaitForever);
        //hi_gpio_unregister_isr_function(IR_RX_PIN);

        uint32_t start, duration;
        uint32_t command = 0;
        int i;

        // --- Header Pulse检测 ---
        // 当前下降沿为Header Pulse起点，等待信号回升，测量低脉宽
        start = hi_get_us();
        while (IoTGpioGetVal(IR_RX_PIN) == 0)
            ; // 忙等待直到信号变高
        duration = hi_get_us() - start;
        if (!within_tolerance(duration, NEC_HDR_PULSE)) {
            // 不是合法的Header脉宽，跳过此次解码
            //printf("不是合法的Header脉宽, 跳过此次解码, duration = %d\n",duration);
            continue;
        }

        // --- Header Space检测 ---
        start = hi_get_us();
        while (IoTGpioGetVal(IR_RX_PIN) == 1)
            ; // 等待信号再次下降
        duration = hi_get_us() - start;
        if (!within_tolerance(duration, NEC_HDR_SPACE)) {
            // Header Space检测失败
            //printf("Header Space检测失败");
            continue;
        }

        // --- 逐位解析32位数据 ---
        for (i = 0; i < 32; i++) {
            // 1. 测量数据位脉冲：低电平应为约560us
            start = hi_get_us();
            while (IoTGpioGetVal(IR_RX_PIN) == 0)
                ; // 等待低电平结束
            duration = hi_get_us() - start;
            if (!within_tolerance(duration, NEC_BIT_PULSE)) {
                // 数据位头部脉冲异常，中止此次解码
                //printf("数据位头部脉冲异常，中止此次解码");
                command = 0;
                break;
            }

            // 2. 测量紧接的高电平：决定数据位值
            start = hi_get_us();
            while (IoTGpioGetVal(IR_RX_PIN) == 1)
                ; // 等待高电平下降，完成该数据位
            duration = hi_get_us() - start;
            if (within_tolerance(duration, NEC_BIT_1_SPACE)) {
                // 高电平较长，判定为1
                command |= (1UL << i);  // 注意NEC协议低位先传输
            }
            else if (within_tolerance(duration, NEC_BIT_0_SPACE)) {
                // 高电平较短，判定为0；此处无需操作command，默认为0
            }
            else {
                // 高电平时长异常，解码出错，中断解析
                //printf("高电平时长异常，解码出错，中断解析");
                command = 0;
                break;
            }
        }

        // 可选：检测结束脉冲（通常为560us），用于确认信号结束
        start = hi_get_us();
        while (IoTGpioGetVal(IR_RX_PIN) == 0)
            ;
        duration = hi_get_us() - start;
        if (!within_tolerance(duration, NEC_END_PULSE)) {
            // 如果结束脉冲异常，可以认为此次命令有误
            // 这里可以选择丢弃或者依然使用command
            //printf("结束脉冲异常，可以认为此次命令有误");
        }

        nec_decoded = command;
        printf("Decoded NEC command: 0x%08X\n", command);
    }
}

void nec_rx_begin_decode(void) {
    osThreadAttr_t rx_attr = {0};
    rx_attr.name = "nec_rx_task";
    rx_attr.stack_size = 1024;
    rx_attr.priority = osPriorityAboveNormal;
    if (osThreadNew(nec_rx_decode_task, NULL, &rx_attr) == NULL) {
        printf("Failed to create NEC receiver task.\n");
    }
}
