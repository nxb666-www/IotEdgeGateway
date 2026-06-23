#include "ESP8266.h"
#include "USART1_Model.h"
#include "Delay.h"
#include <stdio.h>
#include <string.h>

#define ESP8266_WAIT_STEP_MS 10

static uint8_t s_mqtt_ready = 0;
static char s_pending_payload[220];
static uint8_t s_has_pending_payload = 0;

static uint8_t ESP8266_ExtractMqttPayload(const char *rx, char *payload, uint16_t payload_size)
{
    const char *start;
    const char *end;
    uint16_t len;

    if (rx == NULL || payload == NULL || payload_size == 0)
    {
        return 0;
    }

    if (strstr(rx, "+MQTTSUBRECV:") == NULL)
    {
        return 0;
    }

    start = strchr(rx, '{');
    end = strrchr(rx, '}');
    if (start == NULL || end == NULL || end < start)
    {
        return 0;
    }

    len = (uint16_t)(end - start + 1);
    if (len >= payload_size)
    {
        len = payload_size - 1;
    }

    memcpy(payload, start, len);
    payload[len] = '\0';
    return 1;
}

static void ESP8266_SaveMqttPayloadIfAny(void)
{
    if (ESP8266_ExtractMqttPayload((char *)message, s_pending_payload, sizeof(s_pending_payload)))
    {
        s_has_pending_payload = 1;
    }
}

static void ESP8266_ClearRx(void)
{
    ESP8266_SaveMqttPayloadIfAny();
    flag = 0;
    memset(message, 0, sizeof(message));
}

static uint8_t ESP8266_WaitFor(const char *expect, uint16_t timeout_ms)
{
    uint16_t elapsed = 0;

    while (elapsed < timeout_ms)
    {
        if (flag)
        {
            ESP8266_SaveMqttPayloadIfAny();

            if (strstr((char *)message, expect) != NULL)
            {
                flag = 0;
                return 1;
            }

            if (strstr((char *)message, "ERROR") != NULL ||
                strstr((char *)message, "FAIL") != NULL)
            {
                flag = 0;
                return 0;
            }

            flag = 0;
        }

        Delay_ms(ESP8266_WAIT_STEP_MS);
        elapsed += ESP8266_WAIT_STEP_MS;
    }

    return 0;
}

static uint8_t ESP8266_SendCmd(const char *cmd, const char *expect, uint16_t timeout_ms)
{
    ESP8266_ClearRx();
    Send_Str((uint8_t *)cmd);
    Send_Str((uint8_t *)"\r\n");
    return ESP8266_WaitFor(expect, timeout_ms);
}

uint8_t ESP8266_Init(void)
{
    char cmd[140];

    s_mqtt_ready = 0;
    Delay_ms(1000);

    ESP8266_SendCmd("AT", "OK", 1000);
    ESP8266_SendCmd("ATE0", "OK", 1000);
    ESP8266_SendCmd("AT+CWMODE=1", "OK", 1000);

    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ESP8266_WIFI_SSID, ESP8266_WIFI_PASSWORD);
    if (!ESP8266_SendCmd(cmd, "OK", 15000))
    {
        return 0;
    }

    ESP8266_SendCmd("AT+MQTTCLEAN=0", "OK", 1000);

    sprintf(cmd, "AT+MQTTUSERCFG=0,1,\"stm32-f411\",\"\",\"\",0,0,\"\"");
    if (!ESP8266_SendCmd(cmd, "OK", 2000))
    {
        return 0;
    }

    sprintf(cmd, "AT+MQTTCONN=0,\"%s\",%d,1", ESP8266_MQTT_HOST, ESP8266_MQTT_PORT);
    if (!ESP8266_SendCmd(cmd, "OK", 5000))
    {
        return 0;
    }

    if (!ESP8266_SendCmd("AT+MQTTSUB=0,\"iotgw/dev/cmd/#\",0", "OK", 2000))
    {
        return 0;
    }

    s_mqtt_ready = 1;
    return 1;
}

uint8_t ESP8266_IsReady(void)
{
    return s_mqtt_ready;
}

uint8_t ESP8266_Publish(const char *topic, const char *payload)
{
    char cmd[120];

    if (!s_mqtt_ready)
    {
        return 0;
    }

    sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%u,0,0", topic, (unsigned int)strlen(payload));
    if (!ESP8266_SendCmd(cmd, ">", 2000))
    {
        s_mqtt_ready = 0;
        return 0;
    }

    ESP8266_ClearRx();
    Send_Str((uint8_t *)payload);
    if (!ESP8266_WaitFor("OK", 3000))
    {
        s_mqtt_ready = 0;
        return 0;
    }

    return 1;
}

uint8_t ESP8266_TryGetMqttPayload(const char *rx, char *payload, uint16_t payload_size)
{
    return ESP8266_ExtractMqttPayload(rx, payload, payload_size);
}

uint8_t ESP8266_TakeMqttPayload(char *payload, uint16_t payload_size)
{
    uint16_t len;

    if (payload == NULL || payload_size == 0 || !s_has_pending_payload)
    {
        return 0;
    }

    len = (uint16_t)strlen(s_pending_payload);
    if (len >= payload_size)
    {
        len = payload_size - 1;
    }

    memcpy(payload, s_pending_payload, len);
    payload[len] = '\0';
    s_has_pending_payload = 0;
    return 1;
}
