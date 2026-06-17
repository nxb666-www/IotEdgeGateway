#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "stm32f4xx.h"

void Protocol_ParseAndExecute(char *json_str);
void Protocol_UploadAHT30(void);
void Protocol_UploadLightSensor(void);
void Protocol_UploadInfrared(void);

#endif
