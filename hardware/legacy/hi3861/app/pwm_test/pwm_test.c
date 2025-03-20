#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "iot_errno.h"
#include "hi_io.h"

#define PWM_FREQ_DIVITION 64000
#define DELAY_US 25000

static void pwm_task(void) {
    hi_io_set_func(HI_IO_NAME_GPIO_7,HI_IO_FUNC_GPIO_7_PWM0_OUT);
    IoTPwmInit(0);

    int i;

    while(1) {
        for(i = 99; i > 0;i--) {
            IoTPwmStart(0,i,PWM_FREQ_DIVITION);
            usleep(DELAY_US);
            IoTPwmStop(0);
        }
        for(; i < 99;i++) {
            IoTPwmStart(0,i,PWM_FREQ_DIVITION);
            usleep(DELAY_US);
            IoTPwmStop(0);
        }
    }
}

static void pwm_test(void) {
    IoTGpioInit(HI_IO_NAME_GPIO_7);

    osThreadAttr_t attr;
    attr.name = "pwm_test_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024;
    attr.priority = osPriorityNormal;

    if (osThreadNew(pwm_task, NULL, &attr) == NULL) {
        printf("[pwm] Falied to create pwm test task!\n");
    }
}

APP_FEATURE_INIT(pwm_test);