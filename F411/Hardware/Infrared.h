#ifndef __INFRARED_H
#define __INFRARED_H

#include "stm32f4xx.h"

/*
 * 统一语义：Infrared_Read() 返回 1 表示红外触发/有人，0 表示安全。
 * 当前使用红外对射：光路被遮挡才算触发。PA4 开上拉时，常见接收端
 * 光路正常会拉低，遮挡后变高，所以这里按高电平触发。
 */
#define INFRARED_ACTIVE_LOW  0

void Infrared_Init(void);
uint8_t Infrared_Read(void);

#endif
