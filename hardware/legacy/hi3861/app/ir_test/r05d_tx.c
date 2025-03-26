#include <stdint.h>
#include <hi_io.h>
#include <hi_time.h>
#include <iot_gpio.h>
#include <iot_pwm.h>
#include "r05d.h"

#define R05D_PWM_CHANNEL 0
#define R05D_PWM_FREQ 38000
#define R05D_PWM_DUTY 50

#define R05D_TX_PIN HI_IO_NAME_GPIO_9
#define R05D_TX_PIN_FUNC HI_IO_FUNC_GPIO_9_PWM0_OUT

static void r05d_pwm_enable(void) {
    IoTPwmStart(R05D_PWM_CHANNEL,R05D_PWM_DUTY,R05D_PWM_FREQ);
}

static void r05d_pwm_disable(void) {
    IoTPwmStop(R05D_PWM_CHANNEL);
}

static void r05d_send_lead(void) {
    r05d_pwm_enable();
    hi_udelay(R05D_LEAD_PULSE);
    r05d_pwm_disable();
    hi_udelay(R05D_LEAD_SPACE);
}

static void r05d_send_stop(void) {
    r05d_pwm_enable();
    hi_udelay(R05D_STOP_PULSE);
    r05d_pwm_disable();
    hi_udelay(R05D_STOP_SPACE);
}

static void r05d_send_bit_0(void) {
    r05d_pwm_enable();
    hi_udelay(R05D_BIT_0_PULSE);
    r05d_pwm_disable();
    hi_udelay(R05D_BIT_0_SPACE);
}

static void r05d_send_bit_1(void) {
    r05d_pwm_enable();
    hi_udelay(R05D_BIT_1_PULSE);
    r05d_pwm_disable();
    hi_udelay(R05D_BIT_1_SPACE);
}

static void r05d_send_byte(uint8_t data) {
    for(int i = 7;i >= 0;i--) {
        if(data & (1 << i))
            r05d_send_bit_1();
        else
            r05d_send_bit_0();
    }
}

void r05d_tx_init(void) {
    IoTGpioInit(R05D_TX_PIN);
    hi_io_set_func(R05D_TX_PIN,R05D_TX_PIN_FUNC);
    IoTPwmInit(R05D_PWM_CHANNEL);
}

void r05d_tx_send_command(uint8_t a,uint8_t b,uint8_t c) {
    r05d_send_lead();
    r05d_send_byte(a);
    r05d_send_byte(~a);
    r05d_send_byte(b);
    r05d_send_byte(~b);
    r05d_send_byte(c);
    r05d_send_byte(~c);
    r05d_send_stop();

    r05d_send_lead();
    r05d_send_byte(a);
    r05d_send_byte(~a);
    r05d_send_byte(b);
    r05d_send_byte(~b);
    r05d_send_byte(c);
    r05d_send_byte(~c);
    r05d_send_stop();
}