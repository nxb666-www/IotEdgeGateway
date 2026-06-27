#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "stm32f4xx.h"

void Protocol_ParseAndExecute(char *json_str);
void Protocol_UploadSensors(void);
void Protocol_UploadSensorsZigbee(void);

#endif
