#include "Zigbee.h"
#include <string.h>

// Zigbee 模块驱动占位实现
// 当前仅提供空接口，不影响现有 ESP8266 主链路
// 后续接入 Zigbee 协调器时在此实现 USART 通信

static uint8_t zigbee_rx_buf[256];
static volatile uint16_t zigbee_rx_len = 0;
static volatile uint8_t zigbee_has_data = 0;

void Zigbee_Init(void)
{
    // 占位：后续配置 USART2 用于 Zigbee 通信
    // RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    // RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    // ... GPIO/USART 配置 ...
    zigbee_rx_len = 0;
    zigbee_has_data = 0;
}

void Zigbee_Send(const uint8_t *data, uint16_t len)
{
    // 占位：后续通过 USART2 发送数据
    (void)data;
    (void)len;
}

uint8_t Zigbee_HasData(void)
{
    return zigbee_has_data;
}

uint16_t Zigbee_Read(uint8_t *buf, uint16_t max_len)
{
    if (!zigbee_has_data || zigbee_rx_len == 0)
    {
        return 0;
    }

    uint16_t copy_len = zigbee_rx_len;
    if (copy_len > max_len)
    {
        copy_len = max_len;
    }

    memcpy(buf, zigbee_rx_buf, copy_len);
    zigbee_has_data = 0;
    zigbee_rx_len = 0;

    return copy_len;
}
