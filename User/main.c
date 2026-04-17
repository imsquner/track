#include "stm32f10x.h"
#include "OLED.h"
#include "Motor.h"
#include "LightSensor.h"
#include "LED.h"
#include "Buzzer.h"
#include "track.h"
#include "Delay.h"

static char* ZoneText(uint8_t zone)
{
	switch (zone)
	{
		case 0: return "ARC ";
		case 1: return "GRID";
		case 2: return "OBST";
		case 3: return "HORN";
		case 4: return "STOP";
		case 5: return "DONE";
		default: return "UNKN";
	}
}

static char* ActionText(uint8_t action)
{
	switch (action)
	{
		case 0: return "FLW";
		case 1: return "LFT";
		case 2: return "RGT";
		case 3: return "FWD";
		case 4: return "STP";
		default: return "UNK";
	}
}

int main(void)
{
	// Module init
	OLED_Init();
	Motor_Init_A();
	Motor_Init_B();
	LightSensor_Init();
	LED_Init();
	Buzzer_Init();
	Track_Init();
	
	LED1_OFF();
	LED2_OFF();
	Buzzer_OFF();
	
	OLED_Clear();
	
	while(1)
	{
		uint8_t sensorState = LightSensor_GetState();

		Track_Run();

		OLED_ShowString(1,1,"S:");
		OLED_ShowBinNum(1,3, Track_GetFilteredState(), 5);
		OLED_ShowString(1,9,"E:");
		OLED_ShowHexNum(1,11, Track_GetEventFlags(), 1);

		OLED_ShowString(2,1,"Z:");
		OLED_ShowString(2,3, ZoneText(Track_GetZone()));
		OLED_ShowString(2,8,"P:");
		OLED_ShowNum(2,10, Track_GetGridPhase(), 2);

		OLED_ShowString(3,1,"A:");
		OLED_ShowString(3,3, ActionText(Track_GetAction()));
		OLED_ShowString(3,7,"L");
		OLED_ShowSignedNum(3,8, Track_GetSpeedA(), 3);
		OLED_ShowSignedNum(3,12, Track_GetSpeedB(), 3);

		OLED_ShowString(4,1,"R:");
		OLED_ShowBinNum(4,3, sensorState, 5);
		OLED_ShowString(4,9,"F:");
		OLED_ShowBinNum(4,11, Track_GetFilteredState(), 5);
		
		Delay_ms(20);
	}
}
