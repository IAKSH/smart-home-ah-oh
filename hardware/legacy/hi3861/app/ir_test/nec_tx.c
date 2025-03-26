#include "nec.h"
#include <hi_io.h>
#include <hi_time.h>

#define IR_PWM_CHANNEL 0
#define IR_PWM_FREQ 38000  // 38 kHz 载波
#define IR_PWM_DUTY 50

#define MAX_IR_EDGES      100
#define END_GAP_THRESHOLD 10000  // 10ms

#define IR_TX_PIN         HI_IO_NAME_GPIO_9
#define IR_TX_PIN_FUNC    HI_IO_FUNC_GPIO_9_PWM0_OUT

static void nec_pwm_enable(void) {
    IoTPwmStart(IR_PWM_CHANNEL, IR_PWM_DUTY, IR_PWM_FREQ);
}

static void nec_pwm_disable(void) {
    IoTPwmStop(IR_PWM_CHANNEL);
}

static void nec_send_header(void) {
    nec_pwm_enable();
    hi_udelay(NEC_HDR_PULSE);
    nec_pwm_disable();
    hi_udelay(NEC_HDR_SPACE);
}

void nec_tx_init(void) {
    IoTGpioInit(IR_TX_PIN);
    hi_io_set_func(IR_TX_PIN, IR_TX_PIN_FUNC);
    IoTPwmInit(IR_PWM_CHANNEL);
}

void nec_tx_send_bit(uint8_t bit) {
    nec_pwm_enable();
    hi_udelay(NEC_BIT_PULSE);
    nec_pwm_disable();
    if(bit)
        hi_udelay(NEC_BIT_1_SPACE);
    else
        hi_udelay(NEC_BIT_0_SPACE);
}

void nec_tx_send_commond(uint32_t command) {
    nec_send_header();
    for(int i = 0;i < 32;i++) {
        uint8_t bit = (command >> i) & 0x1;
        nec_tx_send_bit(bit);
    }
    nec_pwm_enable();
    hi_udelay(NEC_END_PULSE);
    nec_pwm_disable();
}