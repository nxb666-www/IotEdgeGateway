#ifndef __AHT30_H
#define __AHT30_H

#include "stm32f4xx.h"

extern uint8_t aht30_buf[7];
extern float tem;
extern float hum;

void AHT30_Init(void);
void AHT30_SendMeasure(void);
void AHT30_Receive(void);
void AHT30_ReadData(float *temperature, float *humidity);
unsigned char Calc_CRC8(unsigned char *message, unsigned char Num);
uint8_t AHT30_Caculate(uint8_t *message);

#endif
