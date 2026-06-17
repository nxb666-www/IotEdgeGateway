#include "I2C.h"
#include "Delay.h"

//SCL时钟线操作
//时钟写操作不需要读，时钟控制在主机
void I2C_WriteSCL(uint8_t data){
	GPIO_WriteBit(GPIOB,GPIO_Pin_6,(BitAction)data);
	Delay_us(10);
}

//SDA
//数据线读
void I2C_WriteSDA(uint8_t data){
	GPIO_WriteBit(GPIOB,GPIO_Pin_4,(BitAction)data);
	Delay_us(10);
}
//数据线写
uint8_t I2C_ReadSDA(void){
	uint8_t data = GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4);
	Delay_us(10);
	return data;
}
	

//初始化
void I2C_ModelInit(void){
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	I2C_WriteSCL(1);
    I2C_WriteSDA(1);
		
}
//发送开始信号
void I2C_ModelStart(void){
	//操作时数据线和时钟线都是高电平
	I2C_WriteSDA(1);
	I2C_WriteSCL(1);
	//拉低两条线路
	I2C_WriteSDA(0);
	I2C_WriteSCL(0);
}

//发送结束信号
void I2C_ModelEnd(void){
	I2C_WriteSDA(0);
	I2C_WriteSCL(1);    // SCL 先拉高，SDA 保持低 → STOP 的起始
	I2C_WriteSDA(1);    // SDA 在 SCL 高电平时拉高 → STOP 条件
}
//发送数据
void I2C_ModelSendByte(uint8_t data){
	//拉低时钟不用操作因为默认结束都是低电平
	
	for(uint8_t i=0;i<8;i++){
		//发送数据
		I2C_WriteSDA((data &( 0x80>>i) )? 1:0);
		//拉高时钟线
		I2C_WriteSCL(1);
		//从机接受数据不需要人为操做默认出场就有
		
		//拉低电平
		I2C_WriteSCL(0);
	}
}
//接受数据
uint8_t I2C_ModelReceiveByte(void){
	uint8_t data=0x00;
	
	I2C_WriteSDA(1);
	for(uint8_t i=0;i<8;i++){
		//从机发送数据
	
		//拉高时钟线
		I2C_WriteSCL(1);
		//主机接受数据
		if(I2C_ReadSDA()){
			data |=0x80>>i;
		}
		//拉低电平
		I2C_WriteSCL(0);
	}
	return data;
}
//发送一个应答
void I2C_ModelSendACK(uint8_t ack){
	//拉低时钟不用操作因为默认结束都是低电平
	
	//发送数据
	I2C_WriteSDA(ack);//ack=0  nack=1
	//拉高时钟线
	I2C_WriteSCL(1);
	//从机接受数据不需要人为操做默认出场就有
		
	//拉低电平
	I2C_WriteSCL(0);
}
//接受一个应答
uint8_t I2C_ModelReceiveACK(void){
	uint8_t data;

	I2C_WriteSDA(1);

	//从机发送数据
	//拉高时钟线
	I2C_WriteSCL(1);
	//主机接受数据
	data =I2C_ReadSDA();
	//拉低电平
	I2C_WriteSCL(0);
	return data;
}