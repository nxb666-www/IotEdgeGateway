#include "stm32f4xx.h"
#include "Delay.h"
#include "LED.h"
#include "Motor.h"
#include "AHT30.h"
#include "LightSensor.h"
#include "Buzzer.h"
#include "Infrared.h"
#include "USART1_Model.h"
#include "ESP8266.h"
#include "PWM.h"
#include "protocol.h"

static void ProcessCommand(void)
{
    char payload[200];

    if (ESP8266_TakeMqttPayload(payload, sizeof(payload)))
    {
        Protocol_ParseAndExecute(payload);
        return;
    }

    if (flag == 1)
    {
        if (ESP8266_TryGetMqttPayload((char *)message, payload, sizeof(payload)))
        {
            Protocol_ParseAndExecute(payload);
        }
        else
        {
            Protocol_ParseAndExecute((char*)message);
        }
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

    ESP8266_Init();

    uint16_t aht30_ms = 3000;
    uint16_t light_ms = 800;
    uint16_t infrared_ms = 1600;
    uint16_t esp8266_reconnect_ms = 0;

    while (1)
    {
        ProcessCommand();

        if (!ESP8266_IsReady() && esp8266_reconnect_ms >= 5000)
        {
            esp8266_reconnect_ms = 0;
            ESP8266_Init();
        }

        if (aht30_ms >= 3000)
        {
            aht30_ms = 0;
            Protocol_UploadAHT30();
            ProcessCommand();
        }

        if (light_ms >= 2000)
        {
            light_ms = 0;
            Protocol_UploadLightSensor();
            ProcessCommand();
        }

        if (infrared_ms >= 2000)
        {
            infrared_ms = 0;
            Protocol_UploadInfrared();
            ProcessCommand();
        }

        WaitAndProcess(10);
        aht30_ms += 10;
        light_ms += 10;
        infrared_ms += 10;
        esp8266_reconnect_ms += 10;
    }
}
