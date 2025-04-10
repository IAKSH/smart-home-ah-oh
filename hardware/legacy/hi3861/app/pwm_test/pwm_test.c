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

#define PWM_PIN HI_IO_NAME_GPIO_8
#define PWM_PIN_FUNCTION HI_IO_FUNC_GPIO_8_PWM1_OUT
#define PWM_PORT 1

static void pwm_task(void) {
    IoTGpioInit(PWM_PIN);
    hi_io_set_func(PWM_PIN,PWM_PIN_FUNCTION);
    IoTPwmInit(PWM_PORT);

    int i;

    while(1) {
        for(i = 99; i > 0;i--) {
            IoTPwmStart(PWM_PORT,i,PWM_FREQ_DIVITION);
            usleep(DELAY_US);
            IoTPwmStop(PWM_PORT);
        }
        for(; i < 99;i++) {
            IoTPwmStart(PWM_PORT,i,PWM_FREQ_DIVITION);
            usleep(DELAY_US);
            IoTPwmStop(PWM_PORT);
        }
    }
}

static void pwm_test(void) {
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