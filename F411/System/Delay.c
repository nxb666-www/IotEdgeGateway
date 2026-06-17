#include "Delay.h"

static volatile uint32_t sys_millis = 0;

void SysTick_Handler(void)
{
    sys_millis++;
}

void Time_Init(void)
{
    SysTick_Config(100000);
}

uint32_t millis(void)
{
    return sys_millis;
}

void Delay_us(uint32_t us)
{
    SysTick->LOAD = 100 * us;
    SysTick->VAL = 0;
    SysTick->CTRL = 0x00000005;

    while (!(SysTick->CTRL & 0x00010000));

    SysTick->CTRL = 0x00000004;
}

void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

void Delay_s(uint32_t s)
{
    while (s--)
    {
        Delay_ms(1000);
    }
}
