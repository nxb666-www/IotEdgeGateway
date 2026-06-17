#include "stm32f4xx.h"                  // Device header

//LED1---PA0
void LED_Init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;//无上下拉
	GPIO_Init(GPIOA,&GPIO_InitStruct);

	//初始化的时候，灯是灭的
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
}
//点亮LED1
void LED1_ON(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_0 );
}

void LED1_OFF(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_0 );
}

void LED1_Toggle(void)
{
	GPIO_ToggleBits(GPIOA,GPIO_Pin_0);
}
