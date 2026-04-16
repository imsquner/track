#include "stm32f10x.h"
#include "PWM.h"

// ??A???:PA0 PA3(??) + PA2(PWM) ? ??
void Motor_Init_A(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_3;  // ? ?? PA0 + PA3
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	PWM_Init_A();
}

// ??B???(??)
void Motor_Init_B(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	PWM_Init_B();
}

// ??A????(? ???? PA0?PA3)
void Motor_SetSpeed_A(int8_t Speed)
{
	if (Speed > 100)  Speed = 100;
	if (Speed < -100) Speed = -100;
	
	if (Speed >= 0)
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_0);    // ? PA0
		GPIO_ResetBits(GPIOA, GPIO_Pin_3);   // ? PA3
		PWM_SetCompare3(Speed);
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_0);  // ? PA0
		GPIO_SetBits(GPIOA, GPIO_Pin_3);    // ? PA3
		PWM_SetCompare3(-Speed);
	}
}

// ??B(??)
void Motor_SetSpeed_B(int8_t Speed)
{
	if (Speed > 100)  Speed = 100;
	if (Speed < -100) Speed = -100;
	
	if (Speed >= 0)
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_6);
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);
		PWM_SetCompare2(Speed);
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_6);
		GPIO_SetBits(GPIOA, GPIO_Pin_7);
		PWM_SetCompare2(-Speed);
	}
}
