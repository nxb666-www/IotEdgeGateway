#include "protocol.h"
#include "cJSON.h"
#include "LED.h"
#include "Motor.h"
#include "Buzzer.h"
#include "AHT30.h"
#include "LightSensor.h"
#include "Infrared.h"
#include "USART1_Model.h"
#include "PWM.h"
#include <string.h>
#include <stdio.h>

void Protocol_ParseAndExecute(char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL)
    {
        return;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (type && type->valuestring && strcmp(type->valuestring, "control") == 0)
    {
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload == NULL)
        {
            cJSON_Delete(root);
            return;
        }

        cJSON *led_on = cJSON_GetObjectItem(payload, "led_on");
        cJSON *led_br = cJSON_GetObjectItem(payload, "led_br");
        if (led_on)
        {
            if (led_on->valueint)
            {
                PWM_SetDuty(TIM2, led_br ? led_br->valueint : 50);
            }
            else
            {
                PWM_SetDuty(TIM2, 0);
            }
        }

        cJSON *motor_on = cJSON_GetObjectItem(payload, "motor_on");
        cJSON *motor_sp = cJSON_GetObjectItem(payload, "motor_sp");
        cJSON *motor_dir = cJSON_GetObjectItem(payload, "motor_dir");
        if (motor_on)
        {
            if (motor_on->valueint)
            {
                uint8_t speed = motor_sp ? motor_sp->valueint : 30;
                if (motor_dir && motor_dir->valueint == 1)
                {
                    Motor_Reverse(speed);
                }
                else
                {
                    Motor_Forward(speed);
                }
            }
            else
            {
                Motor_Stop();
            }
        }

        cJSON *buzzer = cJSON_GetObjectItem(payload, "buzzer");
        if (buzzer)
        {
            if (buzzer->valueint)
            {
                Buzzer_On();
            }
            else
            {
                Buzzer_Off();
            }
        }
    }

    cJSON_Delete(root);
}

// 上传AHT30温湿度数据
void Protocol_UploadAHT30(void)
{
    char json[200];
    float temp = 0.0f;
    float humi = 0.0f;

    AHT30_ReadData(&temp, &humi);

    int temp_int = (int)temp;
    int temp_deci = (int)((temp - temp_int) * 10);
    int humi_int = (int)humi;
    int humi_deci = (int)((humi - humi_int) * 10);

    sprintf(json,
        "{\"source\":\"stm32\",\"driver\":\"aht30\",\"id\":1,"
        "\"data\":{\"data_time\":0,\"temp_int\":%d,\"temp_deci\":%d,"
        "\"humi_int\":%d,\"humi_deci\":%d,\"check_sum\":0}}\r\n",
        temp_int, temp_deci, humi_int, humi_deci);

    Send_Str((uint8_t *)json);
}

// 上传光照数据
void Protocol_UploadLightSensor(void)
{
    char json[150];
    uint8_t light = LightSensor_Read();

    sprintf(json,
        "{\"source\":\"stm32\",\"driver\":\"lightsensor\",\"id\":1,"
        "\"data\":{\"light\":%u,\"check_sum\":0}}\r\n",
        light);

    Send_Str((uint8_t *)json);
}

// 上传红外数据
void Protocol_UploadInfrared(void)
{
    char json[150];
    uint8_t ir = Infrared_Read();

    sprintf(json,
        "{\"source\":\"stm32\",\"driver\":\"infrared\",\"id\":1,"
        "\"data\":{\"ir\":%u,\"check_sum\":0}}\r\n",
        ir);

    Send_Str((uint8_t *)json);
}
