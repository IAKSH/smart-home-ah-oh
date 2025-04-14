#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_pwm.h"
#include "iot_gpio.h"
#include "iot_errno.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "hi_timer.h"

#define PWM_FREQ_DIVITION 64000
#define DELAY_US 25000

#define PWM_PIN HI_IO_NAME_GPIO_8
#define PWM_PIN_FUNCTION HI_IO_FUNC_GPIO_8_PWM1_OUT
#define PWM_PORT 1

#define SG90_PIN HI_IO_NAME_GPIO_11
#define SG90_PIN_FUNCTION HI_IO_FUNC_GPIO_11_GPIO
#define PWM_PERIOD_MS   20
 
static osTimerId_t high_timer;
static osTimerId_t low_timer;
static uint32_t pwm_high_us = 0;
static osMutexId_t sg90_mutex;

void sg90_set_angle(float angle) {
    printf("[DEBUG] SG90 set angle: %d\n",(int)angle);
    if(angle >= 0 && angle <= 180) {
        osMutexAcquire(sg90_mutex,osWaitForever);
        pwm_high_us = (2 * (angle / 180) + 0.5) * 1000;
        osMutexRelease(sg90_mutex);
    }
    else
        printf("[DEBUG] invalid SG90 angle: %d\n",angle);
}

void sg90_init(void) {
    IoTGpioInit(SG90_PIN);
    hi_io_set_func(SG90_PIN,SG90_PIN_FUNCTION);
    IoTGpioSetDir(SG90_PIN, HI_GPIO_DIR_OUT);
}

static void sg90_task(void) {
    sg90_init(); uint32_t us;
    while(1) {
        osMutexAcquire(sg90_mutex,osWaitForever);
        us = pwm_high_us;
        osMutexRelease(sg90_mutex);
        IoTGpioSetOutputVal(SG90_PIN,HI_GPIO_VALUE1);
        hi_udelay(us);
        IoTGpioSetOutputVal(SG90_PIN,HI_GPIO_VALUE0);
        //hi_udelay(20000 - us);
        osDelay((20000 - us) / 10000);
    }
}

static void led_pwm_task(void) {
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
    osThreadAttr_t led_task_attr;
    led_task_attr.name = "led_pwm_test_task";
    led_task_attr.attr_bits = 0U;
    led_task_attr.cb_mem = NULL;
    led_task_attr.cb_size = 0U;
    led_task_attr.stack_mem = NULL;
    led_task_attr.stack_size = 1024;
    led_task_attr.priority = osPriorityNormal;
    if (osThreadNew(led_pwm_task, NULL, &led_task_attr) == NULL) {
        printf("[pwm] Falied to create pwm test task!\n");
    }

    sg90_mutex = osMutexNew(NULL);

    osThreadAttr_t sg90_task_attr;
    sg90_task_attr.name = "sg90_task";
    sg90_task_attr.attr_bits = 0U;
    sg90_task_attr.cb_mem = NULL;
    sg90_task_attr.cb_size = 0U;
    sg90_task_attr.stack_mem = NULL;
    sg90_task_attr.stack_size = 1024;
    sg90_task_attr.priority = osPriorityNormal;
    if (osThreadNew(sg90_task, NULL, &sg90_task_attr) == NULL) {
        printf("[pwm] Falied to create sg90_task!\n");
    }
}

APP_FEATURE_INIT(pwm_test);