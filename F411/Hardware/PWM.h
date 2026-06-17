#ifndef _PWM_H_
#define _PWM_H_

#include "stm32f4xx.h"

void TIM2_PWM_Init(void);
void TIM1_PWM_Init(void);
void PWM_SetDuty(TIM_TypeDef *TIMx, uint8_t duty);

#endif
