#pragma once
#include <stdint.h>

#define NEC_HDR_PULSE     9000
#define NEC_HDR_SPACE     4500
#define NEC_BIT_PULSE     560
#define NEC_BIT_0_SPACE   560
#define NEC_BIT_1_SPACE   1690
#define NEC_END_PULSE     560

void nec_tx_init(void);
void nec_tx_send_bit(uint8_t bit);
void nec_tx_send_commond(uint32_t command);

extern uint32_t nec_decoded;
void nec_rx_begin_decode(void);