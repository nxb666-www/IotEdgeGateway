/**
 * protocol.c — STM32 通信协议层
 *
 * 职责：
 *   1. 上行：采集传感器数据 → 拼 JSON → 串口发给 RK3568（serial_bridge 接收）
 *   2. 下行：收到串口 JSON 命令 → 解析 → 控制 LED/电机/蜂鸣器
 *
 * 执行流程：
 *   main.c 调用 Protocol_UploadAHT30()  → 本文件拼 JSON → Send_Str() → USART1 发出
 *   USART1 中断收到 JSON → main.c flag=1 → Protocol_ParseAndExecute() → 控制外设
 */

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

/**
 * 下行控制：解析 JSON 命令并执行
 *
 * 收到的 JSON 格式（由 serial_bridge 发送）：
 *   {"type":"control","payload":{"led_on":1,"led_br":50,"motor_on":1,"motor_sp":30,"buzzer":1}}
 *
 * 执行流程：
 *   serial_bridge 收到 MQTT 命令 → 转换为 STM32 格式 → 串口发送
 *   → USART1 中断接收 → main.c 设置 flag=1
 *   → main.c while 循环检测到 flag → 调用本函数
 *   → cJSON 解析 → 根据字段控制对应外设
 */
void Protocol_ParseAndExecute(char *json_str)
{
    // 用 cJSON 解析 JSON 字符串
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL)
    {
        // JSON 格式错误，直接丢弃
        return;
    }

    // 检查 "type" 字段是否为 "control"
    // 只处理控制命令，其他类型忽略
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (type && type->valuestring && strcmp(type->valuestring, "control") == 0)
    {
        // 提取 "payload" 对象，里面包含各个外设的控制参数
        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (payload == NULL)
        {
            cJSON_Delete(root);
            return;
        }

        // ========== LED 控制 ==========
        // payload 中的字段: led_on(开/关), led_br(亮度 0-100)
        // 跳转到: PWM.c → PWM_SetDuty() → 设置 TIM2 占空比 → LED 亮度变化
        cJSON *led_on = cJSON_GetObjectItem(payload, "led_on");
        cJSON *led_br = cJSON_GetObjectItem(payload, "led_br");
        if (led_on)
        {
            if (led_on->valueint)
            {
                // led_on=1：开灯，设置亮度（没传亮度就默认 50）
                PWM_SetDuty(TIM2, led_br ? led_br->valueint : 50);
            }
            else
            {
                // led_on=0：关灯，亮度设为 0
                PWM_SetDuty(TIM2, 0);
            }
        }

        // ========== 电机（风扇）控制 ==========
        // payload 中的字段: motor_on(开/关), motor_sp(速度 0-100), motor_dir(方向 0=正转 1=反转)
        // 跳转到: Motor.c → Motor_Forward() / Motor_Reverse() / Motor_Stop()
        cJSON *motor_on = cJSON_GetObjectItem(payload, "motor_on");
        cJSON *motor_sp = cJSON_GetObjectItem(payload, "motor_sp");
        cJSON *motor_dir = cJSON_GetObjectItem(payload, "motor_dir");
        if (motor_on)
        {
            if (motor_on->valueint)
            {
                // motor_on=1：开电机
                uint8_t speed = motor_sp ? motor_sp->valueint : 30;
                if (motor_dir && motor_dir->valueint == 1)
                {
                    Motor_Reverse(speed);  // 反转
                }
                else
                {
                    Motor_Forward(speed);  // 正转（默认）
                }
            }
            else
            {
                // motor_on=0：停电机
                Motor_Stop();
            }
        }

        // ========== 蜂鸣器控制 ==========
        // payload 中的字段: buzzer(1=响 0=停)
        // 跳转到: Buzzer.c → Buzzer_On() / Buzzer_Off()
        cJSON *buzzer = cJSON_GetObjectItem(payload, "buzzer");
        if (buzzer)
        {
            if (buzzer->valueint)
            {
                Buzzer_On();   // 响
            }
            else
            {
                Buzzer_Off();  // 停
            }
        }
    }

    // 释放 cJSON 内存（cJSON_Parse 分配的堆内存必须手动释放）
    cJSON_Delete(root);
}

/**
 * 上行数据：上传 AHT30 温湿度
 *
 * 执行流程：
 *   main.c while 循环 → Delay_s(2) → 调用本函数
 *   → AHT30_ReadData() 读传感器 → sprintf 拼 JSON → Send_Str() → USART1 发出
 *   → RK3568 串口桥 serial_bridge.cpp 的 SerialLineReader::Poll() 接收
 *   → HandleSerialJson() 解析 → PublishTelemetry() 发布到 MQTT
 *
 * 发出的 JSON：
 *   {"source":"stm32","driver":"aht30","id":1,
 *    "data":{"data_time":0,"temp_int":28,"temp_deci":4,"humi_int":32,"humi_deci":0,"check_sum":0}}\r\n
 *
 * serial_bridge 收到后转换为统一模型发布到 MQTT：
 *   topic: iotgw/dev/telemetry/temp  payload: {"device_id":"temp","type":"sensor","data":{"value":28.4},"ts":...}
 *   topic: iotgw/dev/telemetry/humi  payload: {"device_id":"humi","type":"sensor","data":{"value":32.0},"ts":...}
 */
void Protocol_UploadAHT30(void)
{
    char json[200];
    float temp = 0.0f;
    float humi = 0.0f;

    // 读 AHT30 传感器（跳转到 AHT30.c → AHT30_ReadData）
    AHT30_ReadData(&temp, &humi);

    // 拆分整数和小数部分（STM32 没有浮点格式化，用整数拼）
    int temp_int = (int)temp;
    int temp_deci = (int)((temp - temp_int) * 10);
    int humi_int = (int)humi;
    int humi_deci = (int)((humi - humi_int) * 10);

    // 用 sprintf 拼 JSON（不用 cJSON，避免堆分配导致串口不输出）
    sprintf(json,
        "{\"source\":\"stm32\",\"driver\":\"aht30\",\"id\":1,"
        "\"data\":{\"data_time\":0,\"temp_int\":%d,\"temp_deci\":%d,"
        "\"humi_int\":%d,\"humi_deci\":%d,\"check_sum\":0}}\r\n",
        temp_int, temp_deci, humi_int, humi_deci);

    // 通过 USART1 发送（跳转到 USART1_Model.c → Send_Str）
    // 发出后 → RK3568 串口桥 serial_bridge.cpp 接收
    Send_Str((uint8_t *)json);
}

/**
 * 上行数据：上传光照传感器
 *
 * 执行流程：同 Protocol_UploadAHT30
 *
 * 发出的 JSON：
 *   {"source":"stm32","driver":"lightsensor","id":1,"data":{"light":70,"check_sum":0}}\r\n
 *
 * serial_bridge 收到后发布到 MQTT：
 *   topic: iotgw/dev/telemetry/light  payload: {"device_id":"light","type":"sensor","data":{"value":70},"ts":...}
 */
void Protocol_UploadLightSensor(void)
{
    char json[150];

    // 读光照传感器（跳转到 LightSensor.c → LightSensor_Read）
    uint8_t light = LightSensor_Read();

    sprintf(json,
        "{\"source\":\"stm32\",\"driver\":\"lightsensor\",\"id\":1,"
        "\"data\":{\"light\":%u,\"check_sum\":0}}\r\n",
        light);

    Send_Str((uint8_t *)json);
}

/**
 * 上行数据：上传红外检测
 *
 * 执行流程：同 Protocol_UploadAHT30
 *
 * 发出的 JSON：
 *   {"source":"stm32","driver":"infrared","id":1,"data":{"ir":0,"check_sum":0}}\r\n
 *
 * serial_bridge 收到后发布到 MQTT：
 *   topic: iotgw/dev/telemetry/ir  payload: {"device_id":"ir","type":"sensor","data":{"value":0},"ts":...}
 */
void Protocol_UploadInfrared(void)
{
    char json[150];

    // 读红外传感器（跳转到 Infrared.c → Infrared_Read）
    uint8_t ir = Infrared_Read();

    sprintf(json,
        "{\"source\":\"stm32\",\"driver\":\"infrared\",\"id\":1,"
        "\"data\":{\"ir\":%u,\"check_sum\":0}}\r\n",
        ir);

    Send_Str((uint8_t *)json);
}
