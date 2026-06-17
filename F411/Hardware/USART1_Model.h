#ifndef __USART1_MODEL_H
#define __USART1_MODEL_H

#include "stm32f4xx.h"
#include <stdio.h>

extern uint8_t message[200];
extern volatile uint8_t flag;

void USART1_Init(void);
void Send_Byte(uint8_t data);
void Send_Str(uint8_t *str);
void Send_Arr(uint8_t *arr, uint16_t len);
int fputc(int ch, FILE *f);

#endif
