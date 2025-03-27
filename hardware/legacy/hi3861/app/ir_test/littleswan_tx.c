#include <hi_io.h>
#include <hi_time.h>
#include <iot_pwm.h>
#include <iot_errno.h>
#include "littleswan.h"

#define IR_PWM_CHANNEL 0
#define IR_PWM_FREQ 38000
#define IR_PWM_DUTY 50

#define LITTLESWAN_TX_PIN HI_IO_NAME_GPIO_9
#define LITTLESWAN_TX_PIN_FUNC HI_IO_FUNC_GPIO_9_PWM0_OUT

static void littleswan_pwm_enable(void) {
    if(IoTPwmStart(IR_PWM_CHANNEL, IR_PWM_DUTY, IR_PWM_FREQ) == IOT_FAILURE) {
        printf("IoTPwmStart ret IOT_FAILURE!\n");
    }
}

static void littleswan_pwm_disable(void) {
    if(IoTPwmStop(IR_PWM_CHANNEL) == IOT_FAILURE) {
        printf("IoTPwmStop ret IOT_FAILURE");
    }
}

void littleswan_tx_init(void) {
    IoTGpioInit(LITTLESWAN_TX_PIN);
    hi_io_set_func(LITTLESWAN_TX_PIN,LITTLESWAN_TX_PIN_FUNC);
    IoTPwmInit(IR_PWM_CHANNEL);
}

void littleswan_tx_send_demo(void) {
    littleswan_pwm_enable();
    osDelay(10000);
}