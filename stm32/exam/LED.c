#include "LED.h"
#include "Rotary.h"
#include "Delay.h"
void LED_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef GPIO_Struct;
	GPIO_Struct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Struct.GPIO_Pin=GPIO_Pin_1;
	GPIO_Struct.GPIO_Speed=GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOA,&GPIO_Struct);
}







