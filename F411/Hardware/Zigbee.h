#ifndef __ZIGBEE_H
#define __ZIGBEE_H

#include "stm32f4xx.h"

// Zigbee 模块驱动占位
// 当前仅提供接口框架，不影响现有 ESP8266 MQTT 主链路
// 后续可接入 Zigbee 协调器（如 CC2531）实现 Zigbee 设备通信

// 初始化 Zigbee 模块（USART2 或其他串口）
void Zigbee_Init(void);

// 发送数据到 Zigbee 网络
//   data: 要发送的数据
//   len: 数据长度
void Zigbee_Send(const uint8_t *data, uint16_t len);

// 检查是否收到 Zigbee 数据
// 返回 1 表示有数据，0 表示无数据
uint8_t Zigbee_HasData(void);

// 读取 Zigbee 收到的数据
//   buf: 接收缓冲区
//   max_len: 缓冲区大小
// 返回实际读取的字节数
uint16_t Zigbee_Read(uint8_t *buf, uint16_t max_len);

#endif
