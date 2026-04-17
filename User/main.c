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
		case 0: return "ZONE_ARC      ";
		case 1: return "ZONE_GRID     ";
		case 2: return "ZONE_OBSTACLE ";
		case 3: return "ZONE_HORN     ";
		case 4:
		case 5: return "ZONE_STOP/DONE";
		default: return "ZONE_UNKNOWN  ";
	}
}

static char* ActionText(uint8_t action)
{
	switch (action)
	{
		case 0: return "STRAIGHT";
		case 1: return "LEFT    ";
		case 2: return "RIGHT   ";
		case 3: return "FORWARD ";
		case 4: return "STOP    ";
		default: return "UNKNOWN ";
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

		/* Part 1: direct 5-bit sensor state display. */
		OLED_ShowString(1,1,"SENSOR:");
		OLED_ShowBinNum(1,8, sensorState, 5);

		/* Part 2: current action state (SYM). */
		OLED_ShowString(2,1,"SYM:");
		OLED_ShowString(2,5, ActionText(Track_GetAction()));

		/* Part 3: current route zone. */
		OLED_ShowString(3,1,ZoneText(Track_GetZone()));

		/* Keep line 4 for speed telemetry. */
		OLED_ShowString(4,1,"SPD:");
		OLED_ShowSignedNum(4,5, Track_GetSpeedA(), 3);
		OLED_ShowSignedNum(4,10, Track_GetSpeedB(), 3);
		
		Delay_ms(20);
	}
}
