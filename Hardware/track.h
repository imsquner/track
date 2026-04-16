#ifndef __TRACK_H
#define __TRACK_H

#include "stm32f10x.h"
#include "Motor.h"
#include "LightSensor.h"

#define STRAIGHT_SPEED  70
#define TURN_SPEED      50

void Track_Run(void);

#endif
