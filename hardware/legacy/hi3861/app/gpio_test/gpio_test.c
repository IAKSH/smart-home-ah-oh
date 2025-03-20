#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"

static void gpio_task(void* arg) {
    (void) arg;

    printf("[gpio_task] startup!\n");

    IoTGpioInit(HI_IO_NAME_GPIO_8);
    hi_io_set_func(HI_IO_NAME_GPIO_8,HI_IO_FUNC_GPIO_8_GPIO);
    IoTGpioSetDir(HI_IO_NAME_GPIO_8,IOT_GPIO_DIR_OUT);

    while(1) {
        IoTGpioSetOutputVal(HI_IO_NAME_GPIO_8,0);
        osDelay(10);
        IoTGpioSetOutputVal(HI_IO_NAME_GPIO_8,1);
        osDelay(290);
    }
}

static void gpio_entry(void) {
    osThreadAttr_t attr = {0};
    attr.name = "gpio_test";
    attr.stack_size = 768;
    attr.priority = osPriorityNormal;
    if(osThreadNew((osThreadFunc_t)gpio_task,NULL,&attr) == NULL) {
        printf("[gpio_test] Failed to create gpio_task!\n");
    }
}

SYS_RUN(gpio_entry);