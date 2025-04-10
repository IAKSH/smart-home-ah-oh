#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "mqtt_ops.h"
#include "iot_mqtt.h"

#define DHT11_TIMEOUT 100    // ms
#define DHT11_OSTIMER_PERIOD 100 // ms

static void dht11_begin_output(void){
    IoTGpioSetDir(HI_IO_NAME_GPIO_10, IOT_GPIO_DIR_OUT);
}

static void dht11_begin_input(void){
    IoTGpioSetDir(HI_IO_NAME_GPIO_10, IOT_GPIO_DIR_IN);
    // 似乎不必要
    // hi_io_set_pull(HI_IO_NAME_GPIO_10, HI_IO_PULL_NONE);
}

static void dht11_pin_set(int level){
    IoTGpioSetOutputVal(HI_IO_NAME_GPIO_10, level);
}


static int dht11_pin_read(void){
    IotGpioValue val;
    IoTGpioGetInputVal(HI_IO_NAME_GPIO_10, &val);
    return val;
}

static void dht11_start(void) {
    dht11_begin_output();
    dht11_pin_set(0);
    hi_udelay(20000);  // 保持低电平至少 18ms
    dht11_pin_set(1);
    hi_udelay(40);     // 高电平保持20-40us
    dht11_begin_input();
}


static char dht11_read_byte(void) {
    unsigned char i = 0;
    unsigned char data = 0;
    for (; i < 8; i++) {
        unsigned int timeout = DHT11_TIMEOUT;
        // 等待从低到高的边沿
        // 带超时检测
        while (dht11_pin_read() == 0 && --timeout > 0);
        if (timeout == 0) {
            printf("[dht11] Timeout while reading byte (low->high transition)\n");
            return -1;
        }
        hi_udelay(50); // 延时确保传感器稳定输出当前位

        data <<= 1;
        if (dht11_pin_read() == 1) {
            data |= 1;
        }

        timeout = DHT11_TIMEOUT;
        // 等待从高到低的跳变
        // 带超时检测
        while (dht11_pin_read() == 1 && --timeout > 0);
        if (timeout == 0) {
            printf("[dht11] Timeout while reading byte (high->low transition)\n");
            return -1;
        }
    }
    return data;
}


osMutexId_t dht11_mutex = NULL;
// 湿度（整数部分、分数部分）及温度（整数部分、分数部分）
unsigned int dht11_data[4] = {0};

static void dht11_update_data(void) {
    unsigned int R_H = 0, R_L = 0, T_H = 0, T_L = 0;
    unsigned char RH = 0, RL = 0, TH = 0, TL = 0, CHECK = 0;

    dht11_start();

    /* 检测传感器响应 */
    if (dht11_pin_read() == 0) {
        unsigned int timeout = DHT11_TIMEOUT;
        /* 等待读取结束低电平 */
        while (dht11_pin_read() == 0 && --timeout > 0);
        if (timeout == 0) {
            printf("[dht11] Timeout waiting for low level end\n");
            return;
        }
        timeout = DHT11_TIMEOUT;
        /* 等待读取结束高电平 */
        while (dht11_pin_read() == 1 && --timeout > 0);
        if (timeout == 0) {
            printf("[dht11] Timeout waiting for high level end\n");
            return;
        }

        R_H = dht11_read_byte();
        R_L = dht11_read_byte();
        T_H = dht11_read_byte();
        T_L = dht11_read_byte();
        CHECK = dht11_read_byte();  // 校验位

        if ((R_H + R_L + T_H + T_L) == CHECK) {
            RH = R_H;
            RL = R_L;
            TH = T_H;
            TL = T_L;
        } else {
            printf("[dht11] Check failed! RH:%d, RL:%d, TH:%d, TL:%d, CHECK:%d\n",
                   R_H, R_L, T_H, T_L, CHECK);
            return;
        }
    } else {
        printf("[dht11] No response from sensor\n");
        return;
    }

    osMutexAcquire(dht11_mutex, osWaitForever);
    dht11_data[0] = RH % 100;
    dht11_data[1] = RL % 100;
    dht11_data[2] = TH % 100;
    dht11_data[3] = TL % 100;
    osMutexRelease(dht11_mutex);
}


static osSemaphoreId_t dht11_sem_id = NULL;
static osTimerId_t dht11_timer_id = NULL;

static void dht11_timer_callback(void *argument) {
    (void)argument;
    if (osSemaphoreRelease(dht11_sem_id) != osOK) {
        printf("[dht11] Failed to release semaphore in timer callback\n");
    }
}

static void dht11_task(void *arg) {
    (void)arg;
    printf("[dht11] Task started!\n");

    IoTGpioInit(HI_IO_NAME_GPIO_10);
    hi_io_set_func(HI_IO_NAME_GPIO_10, HI_IO_FUNC_GPIO_10_GPIO);
    hi_io_set_pull(HI_IO_NAME_GPIO_10, HI_IO_PULL_UP);

    osEventFlagsWait(mqtt_event_flags,0x01,osFlagsWaitAny,osWaitForever);

    dht11_timer_id = osTimerNew(dht11_timer_callback, osTimerPeriodic, NULL, NULL);
    if (dht11_timer_id == NULL) {
        printf("[dht11] Failed to create timer\n");
        return;
    }

    if (osTimerStart(dht11_timer_id, DHT11_OSTIMER_PERIOD) != osOK) {
        printf("[dht11] Failed to start timer\n");
    }

    while (1) {
        osSemaphoreAcquire(dht11_sem_id, osWaitForever);
        dht11_update_data();

        osMutexAcquire(dht11_mutex, osWaitForever);
        //printf("[dht11] Temperature: %d.%d, Humidity: %d.%d\n",
        //       dht11_data[2], dht11_data[3],
        //       dht11_data[0], dht11_data[1]);

        float val = (float)dht11_data[2] + (float)dht11_data[3] / 10.0f;
        enqueue_mqtt("temperature",ATTR_TYPE_FLOAT,&val);

        val = (float)dht11_data[0] + (float)dht11_data[1] / 10.0f;
        enqueue_mqtt("humidity",ATTR_TYPE_FLOAT,&val);

        osMutexRelease(dht11_mutex);
    }
}

static void dht11_entry(void) {
    dht11_mutex = osMutexNew(NULL);
    if (dht11_mutex == NULL) {
        printf("[dht11] Failed to create mutex\n");
        return;
    }

    dht11_sem_id = osSemaphoreNew(1, 0, NULL);
    if (dht11_sem_id == NULL) {
        printf("[dht11] Failed to create semaphore\n");
        return;
    }

    osThreadAttr_t attr = {0};
    attr.name = "dht11_task";
    attr.stack_size = 1024 * 2;
    attr.priority = osPriorityNormal1;
    if (osThreadNew(dht11_task, NULL, &attr) == NULL) {
        printf("[dht11] Failed to create dht11 task\n");
    }
}

SYS_RUN(dht11_entry);
