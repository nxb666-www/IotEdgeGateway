#include "Zigbee.h"
#include <string.h>

static uint8_t zigbee_rx_buf[256];
static volatile uint16_t zigbee_rx_len = 0;
static volatile uint8_t zigbee_has_data = 0;

void Zigbee_Init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    usart.USART_BaudRate = 115200;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &usart);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);

    nvic.NVIC_IRQChannel = USART2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    zigbee_rx_len = 0;
    zigbee_has_data = 0;
}

void Zigbee_Send(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (data == NULL)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, data[i]);
    }
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

void Zigbee_SendString(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    while (*str)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, (uint8_t)*str++);
    }
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

uint8_t Zigbee_HasData(void)
{
    return zigbee_has_data;
}

uint16_t Zigbee_Read(uint8_t *buf, uint16_t max_len)
{
    uint16_t copy_len;

    if (buf == NULL || max_len == 0 || !zigbee_has_data || zigbee_rx_len == 0)
    {
        return 0;
    }

    copy_len = zigbee_rx_len;
    if (copy_len > max_len)
    {
        copy_len = max_len;
    }

    memcpy(buf, zigbee_rx_buf, copy_len);
    zigbee_has_data = 0;
    zigbee_rx_len = 0;

    return copy_len;
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t ch = (uint8_t)USART_ReceiveData(USART2);

        if (!zigbee_has_data)
        {
            if (ch == '\n' || ch == '\r')
            {
                if (zigbee_rx_len > 0)
                {
                    zigbee_rx_buf[zigbee_rx_len] = '\0';
                    zigbee_has_data = 1;
                }
            }
            else if (zigbee_rx_len < sizeof(zigbee_rx_buf) - 1)
            {
                zigbee_rx_buf[zigbee_rx_len++] = ch;
            }
            else
            {
                zigbee_rx_len = 0;
            }
        }

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
