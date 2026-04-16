#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

// ????
void Motor_Init_A(void);
void Motor_Init_B(void);
void Motor_SetSpeed_A(int8_t Speed);
void Motor_SetSpeed_B(int8_t Speed);

#endif
