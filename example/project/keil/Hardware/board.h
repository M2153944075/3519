#ifndef __BOARD_H
#define __BOARD_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

#include "OLED.h"
#include "Encoder.h"
#include "Grayscale.h"
#include "Motor.h"
#include "Track.h"
#include "UART.h"
#include "Bluetooth.h"
#include "PID.h"
#include "JY901.h"

void board_init(void);
void board_update(void);

void delay_us(unsigned long us);
void delay_ms(unsigned long ms);
void delay_1us(unsigned long us);
void delay_1ms(unsigned long ms);

extern volatile uint32_t g_tick_ms;
uint32_t millis(void);

#endif
