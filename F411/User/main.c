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

static void ProcessCommand(void)
{
    if (flag == 1)
    {
        Protocol_ParseAndExecute((char*)message);
        flag = 0;
    }
}

static void WaitAndProcess(uint16_t ms)
{
    while (ms--)
    {
        ProcessCommand();
        Delay_ms(1);
    }
}

int main(void)
{
    TIM1_PWM_Init();

    LED_Init();
    Motor_Init();
    TIM2_PWM_Init();

    AHT30_Init();
    LightSensor_Init();
    Buzzer_Init();
    Infrared_Init();
    USART1_Init();

    Send_Str((uint8_t *)"STM32 Ready\r\n");

    uint16_t aht30_ms = 2000;
    uint16_t light_ms = 200;
    uint16_t infrared_ms = 200;

    while (1)
    {
        ProcessCommand();

        if (aht30_ms >= 2000)
        {
            aht30_ms = 0;
            Protocol_UploadAHT30();
        }

        if (light_ms >= 200)
        {
            light_ms = 0;
            Protocol_UploadLightSensor();
        }

        if (infrared_ms >= 200)
        {
            infrared_ms = 0;
            Protocol_UploadInfrared();
        }

        WaitAndProcess(10);
        aht30_ms += 10;
        light_ms += 10;
        infrared_ms += 10;
    }
}
