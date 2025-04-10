#include <ohos_init.h>
#include <cmsis_os2.h>
#include <stdbool.h>
#include <iot_gpio.h>
#include <hi_io.h>

#define ENABLE_LITTLESWAN_TX_DEMO 1
#define ENABLE_R05D_TX_DEMO 2
#define ENABLE_NEC_TX_DEMO 3

#define ENABLE_IR_DEMO 0

#if ENABLE_IR_DEMO == ENABLE_R05D_TX_DEMO
#include "r05d.h"
#elif ENABLE_IR_DEMO == ENABLE_NEC_TX_DEMO
#include "nec.h"
#elif ENABLE_IR_DEMO == ENABLE_LITTLESWAN_TX_DEMO
#include "littleswan.h"
#endif

static const uint8_t R05D_POWER_ON[3] = {0xB2,0xBF,0x10};
static const uint8_t R05D_POWER_OFF[3] = {0xB2,0x7B,0xE0};
 
bool ir_state = false;

static void send_r05d_command(uint8_t* cmd) {
    r05d_tx_send_command(cmd[0],cmd[1],cmd[2]);
}

#if ENABLE_IR_DEMO != 0
static void ir_tx_demo_task(void *arg) {
    (void)arg;
#if ENABLE_IR_DEMO == ENABLE_NEC_TX_DEMO
    nec_tx_init();
    while (1) {
        nec_tx_send_commond(0x00FF00FF);
        printf("NEC command sent: 0x%08X\n", 0x00FF00FF);
        osDelay(100);
    }
#elif ENABLE_IR_DEMO == ENABLE_R05D_TX_DEMO
    r05d_tx_init();
    uint8_t a = 0xB2;
    //uint8_t b = 0x7B;
    uint8_t b = 0xBF;
    //uint8_t c = 0xE0;
    uint8_t c = 0x10;

    while (1) {
        r05d_tx_send_command(a,b,c);
        printf("R05D command sent: a=0x%08X, b=0x%08X, c=0x%08X\n", a, b, c);
        osDelay(300);
        r05d_tx_send_command(0xB2,0x7B,0xE0);
        osDelay(300);
    }
#elif ENABLE_IR_DEMO == ENABLE_LITTLESWAN_TX_DEMO
    littleswan_tx_init();
    while(1) {
        littleswan_tx_send_demo();
        printf("fuck littleswan\n");
        osDelay(100);
    }
#endif
}
#endif

static void ir_mqtt_handle_task(void) {
    extern osSemaphoreId_t mqtt_ir_sem;
    r05d_tx_init();
    while(1) {
        osSemaphoreAcquire(mqtt_ir_sem,osWaitForever);
        if(ir_state) {
            send_r05d_command(R05D_POWER_ON);
            printf("[DEBUG] R05D send power on");
        }
        else {
            send_r05d_command(R05D_POWER_OFF);
            printf("[DEBUG] R05D send power off");
        }
    }
}

static void ir_app_entry(void) {
#if ENABLE_IR_DEMO != 0
    osThreadAttr_t tx_attr = {0};
    tx_attr.name = "ir_tx_demo_task";
    tx_attr.stack_size = 1024;
    tx_attr.priority = osPriorityNormal2;
    if (osThreadNew(ir_tx_demo_task, NULL, &tx_attr) == NULL) {
        printf("Failed to create IR transmitter task.\n");
    }
#else
    //nec_rx_begin_decode();
    //r05d_rx_begin_decode();

    osThreadAttr_t tx_attr = {0};
    tx_attr.name = "ir_mqtt_handle_task";
    tx_attr.stack_size = 1024;
    tx_attr.priority = osPriorityNormal2;
    if (osThreadNew(ir_mqtt_handle_task, NULL, &tx_attr) == NULL) {
        printf("Failed to create ir_mqtt_handle_task.\n");
    }
#endif
}
 
SYS_RUN(ir_app_entry);
