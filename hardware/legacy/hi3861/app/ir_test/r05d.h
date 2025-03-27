#pragma once
#include <stdint.h>

#define R05D_LEAD_PULSE 4400
#define R05D_LEAD_SPACE 4400
#define R05D_STOP_PULSE 540
#define R05D_STOP_SPACE 5220
#define R05D_BIT_0_PULSE 540
#define R05D_BIT_0_SPACE 540
#define R05D_BIT_1_PULSE 540
#define R05D_BIT_1_SPACE 1620

void r05d_tx_init(void);
void r05d_tx_send_command(uint8_t a,uint8_t b,uint8_t c);

void r05d_rx_begin_decode(void);