#include "stm32f10x.h"
#include "OLED.h"
#include "Motor.h"
#include "LightSensor.h"
#include "track.h"
#include "Delay.h"

int main(void)
{
	// ?????
	OLED_Init();
	Motor_Init_A();
	Motor_Init_B();
	LightSensor_Init();
	
	// OLED????
	OLED_ShowString(1,1,"L1 L2 M R2 R1");
	
	while(1)
	{
		uint8_t Sensor_State = LightSensor_GetState();
		
		// ?????????
		OLED_ShowNum(3,2,  (Sensor_State>>4)&1, 1);
		OLED_ShowNum(3,5,  (Sensor_State>>3)&1, 1);
		OLED_ShowNum(3,8,  (Sensor_State>>2)&1, 1);
		OLED_ShowNum(3,11, (Sensor_State>>1)&1, 1);
		OLED_ShowNum(3,14, Sensor_State&1,      1);
		
		// ??????
		Track_Run();
		
		Delay_ms(20);
	}
}
