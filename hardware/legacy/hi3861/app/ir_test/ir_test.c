#include <ohos_init.h>
#include <cmsis_os2.h>
#include <iot_gpio.h>
#include <hi_io.h>
#include "nec.h"

#ifdef ENBALE_NEC_TX_DEMO
static void ir_tx_demo_task(void *arg) {
    (void)arg;
    nec_tx_init();
    while (1) {
        nec_tx_send_commond(0x00FF00FF);
        printf("NEC command sent: 0x%08X\n", 0x00FF00FF);
        osDelay(100);
    }
}
#endif

static void ir_app_entry(void) {
#ifdef ENBALE_NEC_TX_DEMO
    osThreadAttr_t tx_attr = {0};
    tx_attr.name = "ir_tx_demo_task";
    tx_attr.stack_size = 1024;
    tx_attr.priority = osPriorityNormal;
    if (osThreadNew(ir_tx_demo_task, NULL, &tx_attr) == NULL) {
        printf("Failed to create IR transmitter task.\n");
    }
#else
    nec_rx_begin_decode();
#endif
}

SYS_RUN(ir_app_entry);
