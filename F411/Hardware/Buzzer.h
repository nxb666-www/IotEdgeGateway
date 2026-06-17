#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f4xx.h"

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Beep(uint16_t duration_ms);  // 响指定毫秒后自动关闭

#endif
