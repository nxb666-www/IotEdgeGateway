/**
 * @file I2C.h
 * 软件模拟 I2C 主机驱动 (~100kHz)
 * 引脚: PB6=SCL(时钟线), PB4=SDA(数据线)
 * 原理: 开漏输出+上拉实现"线与"逻辑, 谁都能拉低总线
 * 总线共享: MPU6050(0x68) AHT30(0x38) BMP280(0x76) MAX30102(0x57) CST816(0x15)
 * 互斥锁: 多任务访问需先获取 xI2CMutex (定义在 task_sensor.c)
 */

#ifndef  I2C_H
#define  I2C_H
#include "stm32f4xx.h"                  // Device header
#include "Delay.h"


//SCL 时钟线: 由主机控制, 只写不读
void I2C_WriteSCL(uint8_t data);

//SDA 数据线: 双向, 开漏模式下写1=释放总线(从机可拉低), 写0=拉低
void I2C_WriteSDA(uint8_t data);        // 数据线写 (主机→从机)
uint8_t I2C_ReadSDA(void);             // 数据线读 (从机→主机)

void I2C_ModelInit(void);               // 初始化 GPIO: SCL/SDA 开漏+上拉, 释放总线
void I2C_ModelStart(void);              // START: SCL高时 SDA↓ (通知从机即将通信)
void I2C_ModelEnd(void);               // STOP:  SCL高时 SDA↑ (通知从机通信结束)
void I2C_ModelSendByte(uint8_t data);   // 发送 1 字节, MSB first, 每字节后需跟 ACK
uint8_t I2C_ModelReceiveByte(void);     // 接收 1 字节, MSB first
void I2C_ModelSendACK(uint8_t ack);     // 主机发 ACK(0=继续) / NACK(1=停止)
uint8_t I2C_ModelReceiveACK(void);     // 主机收 ACK: 返回0=从机应答, 1=从机不应答(可能设备不在)

#endif
