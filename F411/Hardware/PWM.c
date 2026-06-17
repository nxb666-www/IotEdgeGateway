#include "PWM.h"

void TIM2_PWM_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_TIM2);

    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_InitStruct.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_Period            = 1000 - 1;
    TIM_InitStruct.TIM_Prescaler         = 100 - 1;
    TIM_InitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_InitStruct);

    TIM_OCInitTypeDef OC_InitStruct;
    OC_InitStruct.TIM_OCMode      = TIM_OCMode_PWM1;
    OC_InitStruct.TIM_OCPolarity  = TIM_OCPolarity_High;
    OC_InitStruct.TIM_OutputState = TIM_OutputState_Enable;
    OC_InitStruct.TIM_Pulse       = 0;
    TIM_OC1Init(TIM2, &OC_InitStruct);

    TIM_Cmd(TIM2, ENABLE);
}

void TIM1_PWM_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);

    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_InitStruct.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_Period            = 500 - 1;
    TIM_InitStruct.TIM_Prescaler         = 100 - 1;
    TIM_InitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_InitStruct);

    TIM_OCInitTypeDef OC_InitStruct;
    OC_InitStruct.TIM_OCMode      = TIM_OCMode_PWM1;
    OC_InitStruct.TIM_OCPolarity  = TIM_OCPolarity_High;
    OC_InitStruct.TIM_OutputState = TIM_OutputState_Enable;
    OC_InitStruct.TIM_Pulse       = 0;
    TIM_OC1Init(TIM1, &OC_InitStruct);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void PWM_SetDuty(TIM_TypeDef *TIMx, uint8_t duty)
{
    if (duty > 100)
    {
        duty = 100;
    }

    TIM_SetCompare1(TIMx, (uint16_t)((duty * (TIMx->ARR + 1)) / 100));
}
