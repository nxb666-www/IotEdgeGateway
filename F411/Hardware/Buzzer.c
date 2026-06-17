#include "Buzzer.h"

void Buzzer_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_SetBits(GPIOB, GPIO_Pin_5);
}

void Buzzer_On(void)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_5);
}

void Buzzer_Off(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_5);
}

void Buzzer_Beep(uint16_t duration_ms)
{
    (void)duration_ms;
}
