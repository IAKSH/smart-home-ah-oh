#include <ohos_init.h>
#include <cmsis_os2.h>
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
    uint8_t b = 0x7B;
    uint8_t c = 0xE0;

    while (1) {
        r05d_tx_send_command(a,b,c);
        printf("R05D command sent: a=0x%08X, b=0x%08X, c=0x%08X\n", a, b, c);
        osDelay(100);
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

static void ir_app_entry(void) {
#if ENABLE_IR_DEMO != 0
    osThreadAttr_t tx_attr = {0};
    tx_attr.name = "ir_tx_demo_task";
    tx_attr.stack_size = 1024;
    tx_attr.priority = osPriorityNormal;
    if (osThreadNew(ir_tx_demo_task, NULL, &tx_attr) == NULL) {
        printf("Failed to create IR transmitter task.\n");
    }
#else
    nec_rx_begin_decode();
    //r05d_rx_begin_decode();
#endif
}
 
SYS_RUN(ir_app_entry);
