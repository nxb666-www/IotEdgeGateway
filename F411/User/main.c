#include "stm32f4xx.h"
#include "Delay.h"
#include "LED.h"
#include "Motor.h"
#include "AHT30.h"
#include "LightSensor.h"
#include "Buzzer.h"
#include "Infrared.h"
#include "USART1_Model.h"
#include "PWM.h"
#include "protocol.h"

int main(void)
{
    // 1. 初始化PWM（电机）
    TIM1_PWM_Init();   // 电机 PWM (PA8)

    // 2. 初始化外设
    LED_Init();
    LED1_ON();
    Motor_Init();
    AHT30_Init();
    LightSensor_Init();
    Buzzer_Init();
    Infrared_Init();
    USART1_Init();

    // 3. 测试串口
    Send_Str((uint8_t *)"STM32 Ready\r\n");

    while(1)
    {
        // 4. 处理串口收到的JSON命令
        if (flag == 1)
        {
            Protocol_ParseAndExecute((char*)message);
            flag = 0;
        }

        // 5. 轮流上传传感器数据（每个间隔2秒）
        Delay_s(2);
        Protocol_UploadAHT30();
        Delay_s(2);
        Protocol_UploadLightSensor();
        Delay_s(2);
        Protocol_UploadInfrared();
    }
}
