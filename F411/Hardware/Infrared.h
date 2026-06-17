#ifndef __INFRARED_H
#define __INFRARED_H

#include "stm32f4xx.h"

void Infrared_Init(void);
uint8_t Infrared_Read(void);  // 返回0或1
uint32_t Infrared_GetCount(void);  // 获取计数
void Infrared_ResetCount(void);    // 重置计数

#endif
