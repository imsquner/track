#include "track.h"

// ??
static void Car_Straight(void)
{
	Motor_SetSpeed_A(STRAIGHT_SPEED);
	Motor_SetSpeed_B(STRAIGHT_SPEED);
}

// ??(????)
static void Car_Right(void)
{
	Motor_SetSpeed_A(STRAIGHT_SPEED);
	Motor_SetSpeed_B(TURN_SPEED);
}

// ??(????)
static void Car_Left(void)
{
	Motor_SetSpeed_A(TURN_SPEED);
	Motor_SetSpeed_B(STRAIGHT_SPEED);
}

// ??
static void Car_Stop(void)
{
	Motor_SetSpeed_A(0);
	Motor_SetSpeed_B(0);
}

// ??????(? ???:1=??,0=??)
void Track_Run(void)
{
	uint8_t state = LightSensor_GetState();
	// ??? ??:???? + ???5?(5????)
	
	switch(state)
	{
		case 0x04: Car_Straight(); break;
		case 0x08: Car_Right();    break;
		case 0x10: Car_Right();    break;
		case 0x02: Car_Left();     break;
		case 0x01: Car_Left();     break;
		case 0x00: Car_Straight(); break;
		
		// 2??????
		case 0x0C: Car_Right();    break;
		case 0x18: Car_Right();    break;
		case 0x06: Car_Left();     break;
		case 0x03: Car_Left();     break;
		
		// ????
		default: Car_Stop(); break;
	}
}
