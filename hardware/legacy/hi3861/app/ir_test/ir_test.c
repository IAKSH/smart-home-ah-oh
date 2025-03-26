#include <ohos_init.h>
#include <cmsis_os2.h>
#include <iot_gpio.h>
#include <hi_io.h>

#define ENABLE_R05D_TX_DEMO

#if defined ENABLE_R05D_TX_DEMO
#include "r05d.h"
#elif defined ENABLE_NEC_TX_DEMO
#include "nec.h"
#endif


#if defined ENABLE_R05D_TX_DEMO || ENABLE_NEC_TX_DEMO
static void ir_tx_demo_task(void *arg) {
    (void)arg;
#ifdef ENABLE_NEC_TX_DEMO
    nec_tx_init();
    while (1) {
        nec_tx_send_commond(0x00FF00FF);
        printf("NEC command sent: 0x%08X\n", 0x00FF00FF);
        osDelay(100);
    }
#else
    r05d_tx_init();
    uint8_t a = 0xB2;
    uint8_t b = 0x9F;
    uint8_t c = 0x00;
    while (1) {
        r05d_tx_send_command(a,b,c);
        printf("R05D command sent: a=0x%08X, b=0x%08X, c=0x%08X\n", a, b, c);
        osDelay(100);
    }
#endif
}
#endif


static void ir_app_entry(void) {
#if defined ENABLE_R05D_TX_DEMO || ENABLE_NEC_TX_DEMO
    osThreadAttr_t tx_attr = {0};
    tx_attr.name = "ir_tx_demo_task";
    tx_attr.stack_size = 1024;
    tx_attr.priority = osPriorityNormal;
    if (osThreadNew(ir_tx_demo_task, NULL, &tx_attr) == NULL) {
        printf("Failed to create IR transmitter task.\n");
    }
#elif defined ENABLE_NEC_RX
    nec_rx_begin_decode();
#endif
}

SYS_RUN(ir_app_entry);
