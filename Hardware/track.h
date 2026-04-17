#ifndef __TRACK_H
#define __TRACK_H

#include "stm32f10x.h"
#include "Motor.h"
#include "LightSensor.h"

#define STRAIGHT_SPEED  82
#define TURN_SPEED      62

void Track_Init(void);
void Track_Run(void);
uint8_t Track_GetFilteredState(void);
uint8_t Track_GetZone(void);
uint8_t Track_GetAction(void);
uint8_t Track_GetGridPhase(void);
uint8_t Track_GetEventFlags(void);
int8_t Track_GetSpeedA(void);
int8_t Track_GetSpeedB(void);

#endif
