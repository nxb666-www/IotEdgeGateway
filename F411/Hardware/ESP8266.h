#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f4xx.h"

#define ESP8266_WIFI_SSID      "nie"
#define ESP8266_WIFI_PASSWORD  "12345678"
#define ESP8266_MQTT_HOST      "192.168.233.107"
#define ESP8266_MQTT_PORT      1884

uint8_t ESP8266_Init(void);
uint8_t ESP8266_IsReady(void);
uint8_t ESP8266_Publish(const char *topic, const char *payload);
uint8_t ESP8266_TakeMqttPayload(char *payload, uint16_t payload_size);
uint8_t ESP8266_TryGetMqttPayload(const char *rx, char *payload, uint16_t payload_size);

#endif
