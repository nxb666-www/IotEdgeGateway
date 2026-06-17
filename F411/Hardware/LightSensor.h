#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

#include "stm32f4xx.h"

void LightSensor_Init(void);
uint8_t LightSensor_Read(void);      // 返回光照百分比: 0~100

#endif
