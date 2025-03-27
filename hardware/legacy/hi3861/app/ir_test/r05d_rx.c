
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <hi_io.h>
#include <hi_time.h>
#include <iot_gpio.h>
#include <cmsis_os2.h>
#include "r05d.h"

// 允许±20%的误差
#define TOLERANCE_PERCENT    20

// 判断测量值是否在预期范围内
static inline bool within_tolerance(uint32_t measured, uint32_t expected) {
    uint32_t lower = expected * (100 - TOLERANCE_PERCENT) / 100;
    uint32_t upper = expected * (100 + TOLERANCE_PERCENT) / 100;
    return (measured >= lower) && (measured <= upper);
}

// R05D 协议时序定义
#define R05D_LEAD_PULSE    4400   // 起始低电平脉冲 ~4400us
#define R05D_LEAD_SPACE    4400   // 起始高电平 ~4400us
#define R05D_STOP_PULSE    540    // 停止低脉冲 ~540us
#define R05D_STOP_SPACE    5220   // 停止高脉冲 ~5220us
#define R05D_BIT_0_PULSE   540    // 数据位低电平脉冲 ~540us
#define R05D_BIT_0_SPACE   540    // 数据位0对应高电平 ~540us
#define R05D_BIT_1_PULSE   540    // 数据位低电平脉冲 ~540us
#define R05D_BIT_1_SPACE   1620   // 数据位1对应高电平 ~1620us

// 全局变量：存储解码得到的32位命令
uint32_t r05d_decoded;

// 以下引脚初始化和中断处理与 NEC 版相同，此处直接沿用（如有需要可改为平台化的实现）
#define IR_RX_PIN         HI_IO_NAME_GPIO_12
#define IR_RX_PIN_FUNC    HI_IO_FUNC_GPIO_12_GPIO

static osSemaphoreId_t r05d_rx_sem;

// 中断服务函数：当检测到下降沿时释放信号量
static void r05d_rx_irq_handler(void *arg) {
    osSemaphoreRelease(r05d_rx_sem);
}

static void r05d_rx_init(void) {
    r05d_rx_sem = osSemaphoreNew(1, 0, NULL);
    IoTGpioInit(IR_RX_PIN);
    IoTGpioSetDir(IR_RX_PIN, IOT_GPIO_DIR_IN);
    hi_io_set_func(IR_RX_PIN, IR_RX_PIN_FUNC);
    IoTGpioRegisterIsrFunc(IR_RX_PIN, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, r05d_rx_irq_handler, NULL);
}

static IotGpioValue IoTGpioGetVal(int gpio_id) {
    static IotGpioValue val;
    IoTGpioGetInputVal(gpio_id, &val);
    return val;
}

/*
 * R05D 协议解码流程：
 * 1. 等待由中断唤醒，将本次下降沿作为信号起点（Lead Pulse开始）。
 * 2. 测量低电平持续时长，判断是否大致为4400us的Lead Pulse。
 * 3. 测量高电平（Lead Space，约4400us）。
 * 4. 逐位解析32位数据：
 *    a. 每个数据位首先检测低电平脉冲（应为约540us）。
 *    b. 紧接着测量高电平长度，根据高电平在大约540us或1620us之间判断数据位为0或1（R05D数据通常低位先传输）。
 * 5. 检测停止信号：Stop Pulse（540us）和 Stop Space（5220us）。
 * 6. 将所得32位解码命令存储到全局变量 r05d_decoded 中。
 */
static void r05d_rx_decode_task(void* args) {
    r05d_rx_init();
    
    while (1) {
        // 阻塞等待下降沿中断（Lead Pulse开始）
        osSemaphoreAcquire(r05d_rx_sem, osWaitForever);
        
        uint32_t start, duration;
        uint32_t command = 0;
        int i;
        
        // --- Lead Pulse检测 ---
        start = hi_get_us();
        while (IoTGpioGetVal(IR_RX_PIN) == 0)
            ;  // 忙等待直至信号回升
        duration = hi_get_us() - start;
        if (!within_tolerance(duration, R05D_LEAD_PULSE)) {
            printf("不是合法的 Lead 脉冲, 跳过此次解码, duration = %d\n", duration);
            continue;
        }
        
        // --- Lead Space检测 ---
        start = hi_get_us();
        while (IoTGpioGetVal(IR_RX_PIN) == 1)
            ;  // 等待信号再次下降
        duration = hi_get_us() - start;
        if (!within_tolerance(duration, R05D_LEAD_SPACE)) {
            printf("Lead Space检测失败, duration = %d\n", duration);
            continue;
        }
        
        // --- 逐位解析32位数据 ---
        for (i = 0; i < 32; i++) {
            // 1. 测量数据位低电平脉冲（期望约540us）
            start = hi_get_us();
            while (IoTGpioGetVal(IR_RX_PIN) == 0)
                ;
            duration = hi_get_us() - start;
            if (!within_tolerance(duration, R05D_BIT_0_PULSE)) {
                printf("数据位脉冲异常, 第%d位, duration = %d\n", i, duration);
                command = 0;
                break;
            }
            
            // 2. 测量紧随其后的高电平，判断该数据位值
            start = hi_get_us();
            while (IoTGpioGetVal(IR_RX_PIN) == 1)
                ;
            duration = hi_get_us() - start;
            if (within_tolerance(duration, R05D_BIT_1_SPACE)) {
                // 高电平较长，判定为1（注意数据按低位先传输）
                command |= (1UL << i);
            } else if (within_tolerance(duration, R05D_BIT_0_SPACE)) {
                // 高电平较短，判定为0，command无需额外赋值
            } else {
                printf("数据位高电平异常, 第%d位, duration = %d\n", i, duration);
                command = 0;
                break;
            }
        }
        
        // --- 停止信号检测 ---
        // Stop Pulse检测
        start = hi_get_us();
        while (IoTGpioGetVal(IR_RX_PIN) == 0)
            ;
        duration = hi_get_us() - start;
        if (!within_tolerance(duration, R05D_STOP_PULSE)) {
            printf("停止脉冲异常, duration = %d\n", duration);
        }
        // Stop Space检测
        start = hi_get_us();
        while (IoTGpioGetVal(IR_RX_PIN) == 1)
            ;
        duration = hi_get_us() - start;
        if (!within_tolerance(duration, R05D_STOP_SPACE)) {
            printf("停止间隔异常, duration = %d\n", duration);
        }
        
        r05d_decoded = command;
        printf("Decoded R05D command: 0x%08X\n", command);
    }
}

void r05d_rx_begin_decode(void) {
    osThreadAttr_t rx_attr = {0};
    rx_attr.name = "r05d_rx_task";
    rx_attr.stack_size = 1024;
    rx_attr.priority = osPriorityAboveNormal;
    
    if (osThreadNew(r05d_rx_decode_task, NULL, &rx_attr) == NULL) {
        printf("Failed to create R05D receiver task.\n");
    }
}
