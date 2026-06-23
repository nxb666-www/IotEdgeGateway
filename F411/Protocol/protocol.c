#include "protocol.h"
#include "LED.h"
#include "Motor.h"
#include "Buzzer.h"
#include "AHT30.h"
#include "LightSensor.h"
#include "Infrared.h"
#include "USART1_Model.h"
#include "ESP8266.h"
#include "PWM.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t Json_GetInt(const char *json, const char *key, int *value)
{
    char name[24];
    const char *pos;
    const char *colon;

    sprintf(name, "\"%s\"", key);
    pos = strstr(json, name);
    if (pos == NULL)
    {
        return 0;
    }

    colon = strchr(pos + strlen(name), ':');
    if (colon == NULL)
    {
        return 0;
    }

    colon++;
    while (*colon == ' ' || *colon == '\t')
    {
        colon++;
    }

    *value = atoi(colon);
    return 1;
}

void Protocol_ParseAndExecute(char *json_str)
{
    int led_on;
    int led_br;
    int motor_on;
    int motor_sp;
    int motor_dir;
    int buzzer;

    if (json_str == NULL)
    {
        return;
    }

    if (strstr(json_str, "\"type\"") == NULL || strstr(json_str, "\"control\"") == NULL)
    {
        return;
    }

    if (Json_GetInt(json_str, "led_on", &led_on))
    {
        if (led_on)
        {
            if (!Json_GetInt(json_str, "led_br", &led_br))
            {
                led_br = 50;
            }
            PWM_SetDuty(TIM2, (uint8_t)led_br);
        }
        else
        {
            PWM_SetDuty(TIM2, 0);
        }
    }

    if (Json_GetInt(json_str, "motor_on", &motor_on))
    {
        if (motor_on)
        {
            if (!Json_GetInt(json_str, "motor_sp", &motor_sp))
            {
                motor_sp = 30;
            }
            if (!Json_GetInt(json_str, "motor_dir", &motor_dir))
            {
                motor_dir = 0;
            }

            if (motor_dir == 1)
            {
                Motor_Reverse((uint8_t)motor_sp);
            }
            else
            {
                Motor_Forward((uint8_t)motor_sp);
            }
        }
        else
        {
            Motor_Stop();
        }
    }

    if (Json_GetInt(json_str, "buzzer", &buzzer))
    {
        if (buzzer)
        {
            Buzzer_On();
        }
        else
        {
            Buzzer_Off();
        }
    }
}


void Protocol_UploadAHT30(void)
{
    char payload[180];
    float temp = 0.0f;
    float humi = 0.0f;

    AHT30_ReadData(&temp, &humi);

    int temp_int = (int)temp;
    int temp_deci = (int)((temp - temp_int) * 10);
    int humi_int = (int)humi;
    int humi_deci = (int)((humi - humi_int) * 10);

    if (temp_deci < 0) temp_deci = -temp_deci;
    if (humi_deci < 0) humi_deci = -humi_deci;

    sprintf(payload,
        "{\"device_id\":\"temp\",\"type\":\"sensor\",\"source\":\"stm32\","
        "\"driver\":\"aht30\",\"data\":{\"value\":%d.%d},\"ts\":0}",
        temp_int, temp_deci);
    ESP8266_Publish("iotgw/dev/telemetry/temp", payload);

    sprintf(payload,
        "{\"device_id\":\"humi\",\"type\":\"sensor\",\"source\":\"stm32\","
        "\"driver\":\"aht30\",\"data\":{\"value\":%d.%d},\"ts\":0}",
        humi_int, humi_deci);
    ESP8266_Publish("iotgw/dev/telemetry/humi", payload);
}

void Protocol_UploadLightSensor(void)
{
    char payload[150];

    uint8_t light = LightSensor_Read();

    sprintf(payload,
        "{\"device_id\":\"light\",\"type\":\"sensor\",\"source\":\"stm32\","
        "\"driver\":\"lightsensor\",\"data\":{\"value\":%u},\"ts\":0}",
        (unsigned int)light);

    ESP8266_Publish("iotgw/dev/telemetry/light", payload);
}

void Protocol_UploadInfrared(void)
{
    char payload[150];

    uint8_t ir = Infrared_Read();

    sprintf(payload,
        "{\"device_id\":\"ir\",\"type\":\"sensor\",\"source\":\"stm32\","
        "\"driver\":\"infrared\",\"data\":{\"value\":%u},\"ts\":0}",
        (unsigned int)ir);

    ESP8266_Publish("iotgw/dev/telemetry/ir", payload);
}
