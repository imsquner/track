#ifndef __PWM_H
#define __PWM_H

#include "stm32f10x.h"

// ????A:PA2 - TIM2_CH3
void PWM_Init_A(void);
// ????B:PA1 - TIM2_CH2
void PWM_Init_B(void);

// ???????
void PWM_SetCompare2(uint16_t Compare);
void PWM_SetCompare3(uint16_t Compare);

#endif

