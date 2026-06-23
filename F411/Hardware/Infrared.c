#include "Infrared.h"

void Infrared_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

uint8_t Infrared_Read(void)
{
    uint8_t level = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4);

#if INFRARED_ACTIVE_LOW
    return level == Bit_RESET ? 1 : 0;
#else
    return level == Bit_SET ? 1 : 0;
#endif
}
