#include "AHT30.h"
#include "I2C.h"
#include "Delay.h"

void AHT30_Init(void)
{
	I2C_ModelInit();
	Delay_ms(40);  // 上电等待40ms
	AHT30_SendMeasure();
	AHT30_Receive();
	AHT30_Caculate(aht30_buf);
}

void AHT30_ReadData(float *temperature, float *humidity)
{
	AHT30_SendMeasure();
	AHT30_Receive();
	AHT30_Caculate(aht30_buf);
	*temperature = tem;
	*humidity = hum;
}

//================发送测量命令
void AHT30_SendMeasure(void){
	Delay_ms(5);

	I2C_ModelStart();
	I2C_ModelSendByte(0x70);
	I2C_ModelReceiveACK();
	I2C_ModelSendByte(0xAC);
	I2C_ModelReceiveACK();
	I2C_ModelSendByte(0x33);
	I2C_ModelReceiveACK();
	I2C_ModelSendByte(0x00);
	I2C_ModelReceiveACK();
	I2C_ModelEnd();

	Delay_ms(80);
}

uint8_t aht30_buf[7]={0};
//================获取温湿度校准数据
void AHT30_Receive(void){
	I2C_ModelStart();
	I2C_ModelSendByte(0x71);
	I2C_ModelReceiveACK();

	for(uint8_t i=0;i<7;i++){
		aht30_buf[i]=I2C_ModelReceiveByte();
		if(i<6)
			I2C_ModelSendACK(0);
		else
			I2C_ModelSendACK(1);
	}
	I2C_ModelEnd();
}


//====================CRC校验
unsigned char Calc_CRC8(unsigned char *message,unsigned char Num){
	unsigned char i;
	unsigned char byte;
	unsigned char crc =0xFF;
	for (byte = 0;byte<Num;byte++){
		crc^=(message[byte]);
		for(i=8;i>0;--i){
		if(crc&0x80)
			crc=(crc<<1)^0x31;
		else
			crc=(crc<<1);
		}
	}
	return crc;
}


float tem=0;
float hum=0;

//计算温湿度的值
uint8_t AHT30_Caculate(uint8_t *message){
	//判断校验位是否和校验结果一致
	uint8_t after=Calc_CRC8(message,6);
	if(after!=message[6]){
		return 0;
	}

	//判断状态字节是否忙碌以及是否校准
	if((message[0]&0x80)||!(message[0]&0x08)) {
		return 0;
	}

	//读取温度和湿度
	uint32_t h= message[1]<<12|message[2]<<4|message[3]>>4;
	uint32_t t=(message[3]&0x0F)<<16|message[4]<<8|message[5];

	hum = (float)h / 1048576 * 100;
	tem = (float)t / 1048576 * 200 - 50;
	return 1;
}
