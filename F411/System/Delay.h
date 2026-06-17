#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"

void Time_Init(void);
uint32_t millis(void);
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);

#endif
