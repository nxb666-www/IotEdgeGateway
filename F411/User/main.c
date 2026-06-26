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
#include <string.h>

static void ProcessCommand(void)
{
    if (flag == 1)
    {
        char cmd[300];
        strncpy(cmd, (char *)message, sizeof(cmd) - 1);
        cmd[sizeof(cmd) - 1] = '\0';
        flag = 0;
        Protocol_ParseAndExecute(cmd);
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

    uint16_t sensor_ms = 1000;

    while (1)
    {
        ProcessCommand();

        if (sensor_ms >= 1000)
        {
            sensor_ms = 0;
            Protocol_UploadSensors();
            ProcessCommand();
        }

        WaitAndProcess(5);
        sensor_ms += 5;
    }
}
