#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
//#include "hi_time.h"

static void ir_task(void* arg) {
    (void) arg;

    printf("[ir] startup!\n");

    IoTGpioInit(HI_IO_NAME_GPIO_9);
    hi_io_set_func(HI_IO_NAME_GPIO_9,HI_IO_FUNC_GPIO_9_GPIO);
    IoTGpioSetDir(HI_IO_NAME_GPIO_9,IOT_GPIO_DIR_OUT);

    while(1) {
        IoTGpioSetOutputVal(HI_IO_NAME_GPIO_9,1);
        osDelay(1);
        IoTGpioSetOutputVal(HI_IO_NAME_GPIO_9,0);
        osDelay(99);
    }
}

static void ir_entry(void) {
    osThreadAttr_t attr = {0};
    attr.name = "ir_test";
    attr.stack_size = 768;
    attr.priority = osPriorityNormal;
    if(osThreadNew((osThreadFunc_t)ir_task,NULL,&attr) == NULL) {
        printf("[ir] Failed to create ir_task!\n");
    }
}

SYS_RUN(ir_entry);