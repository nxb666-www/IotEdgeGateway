#include "Motor.h"
#include "PWM.h"

void Motor_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);
    PWM_SetDuty(TIM1, 0);
}

void Motor_Forward(uint8_t speed)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_7);
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);
    PWM_SetDuty(TIM1, speed);
}

void Motor_Reverse(uint8_t speed)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    GPIO_SetBits(GPIOA, GPIO_Pin_1);
    PWM_SetDuty(TIM1, speed);
}

void Motor_Stop(void)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);
    PWM_SetDuty(TIM1, 0);
}

void Motor_Brake(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_7);
    GPIO_SetBits(GPIOA, GPIO_Pin_1);
    PWM_SetDuty(TIM1, 100);
}
