#include "protocol.h"
#include "LED.h"
#include "Motor.h"
#include "Buzzer.h"
#include "AHT30.h"
#include "LightSensor.h"
#include "Infrared.h"
#include "USART1_Model.h"
#include "Zigbee.h"
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


void Protocol_UploadSensors(void)
{
    char payload[96];
    float temp = 0.0f;
    float humi = 0.0f;
    uint8_t light;
    uint8_t ir;

    AHT30_ReadData(&temp, &humi);
    light = LightSensor_Read();
    ir = Infrared_Read();

    int temp_int = (int)temp;
    int temp_deci = (int)((temp - temp_int) * 10);
    int humi_int = (int)humi;
    int humi_deci = (int)((humi - humi_int) * 10);

    if (temp_deci < 0) temp_deci = -temp_deci;
    if (humi_deci < 0) humi_deci = -humi_deci;

    sprintf(payload,
            "{\"temp\":%d.%d,\"humi\":%d.%d,\"light\":%u,\"ir\":%u}\n",
            temp_int, temp_deci,
            humi_int, humi_deci,
            (unsigned int)light,
            (unsigned int)ir);
    Send_Str((uint8_t *)payload);
}

static void Zigbee_SendSensorValue(const char *id, const char *driver, float value)
{
    char payload[128];
    int int_part = (int)value;
    int deci_part = (int)((value - int_part) * 10);

    if (deci_part < 0) deci_part = -deci_part;

    sprintf(payload,
            "{\"device_id\":\"%s\",\"type\":\"sensor\",\"source\":\"stm32\",\"driver\":\"%s\",\"data\":{\"value\":%d.%d},\"ts\":0}\n",
            id, driver, int_part, deci_part);
    Zigbee_SendString(payload);
}

static void Zigbee_SendSensorInt(const char *id, const char *driver, uint8_t value)
{
    char payload[128];

    sprintf(payload,
            "{\"device_id\":\"%s\",\"type\":\"sensor\",\"source\":\"stm32\",\"driver\":\"%s\",\"data\":{\"value\":%u},\"ts\":0}\n",
            id, driver, (unsigned int)value);
    Zigbee_SendString(payload);
}

void Protocol_UploadSensorsZigbee(void)
{
    float temp = 0.0f;
    float humi = 0.0f;
    uint8_t light;
    uint8_t ir;

    AHT30_ReadData(&temp, &humi);
    light = LightSensor_Read();
    ir = Infrared_Read();

    Zigbee_SendSensorValue("temp", "aht30", temp);
    Zigbee_SendSensorValue("humi", "aht30", humi);
    Zigbee_SendSensorInt("light", "lightsensor", light);
    Zigbee_SendSensorInt("ir", "infrared", ir);
}
