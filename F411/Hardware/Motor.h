#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f4xx.h"

void Motor_Init(void);
void Motor_Forward(uint8_t speed);   // 正转，speed: 0~100
void Motor_Reverse(uint8_t speed);   // 反转，speed: 0~100
void Motor_Stop(void);               // 停止
void Motor_Brake(void);              // 制动

#endif
