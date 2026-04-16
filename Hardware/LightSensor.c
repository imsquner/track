#include "stm32f10x.h"                  // Device header

/**
  * @brief  5?????????(??PB3??)
  * @??   ?1-PB4  ?2-PB3  ?-PB15  ?2-PB14  ?1-PB13
  */
void LightSensor_Init(void)
{
    // 1. ?????AFIO??(???????)?GPIOB??
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);
    
    // 2. ??JTAG??,??PB3/PB4/PA15,??SWD??(????)
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   // ??????
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
  * @brief  ??5????????
  * @retval ?????  bit4-?1  bit3-?2  bit2-?  bit1-?2  bit0-?1
  */
uint8_t LightSensor_GetState(void)
{
    uint8_t State = 0;
    
    State |= GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)  << 4;  // ?1 ? bit4
    State |= GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3)  << 3;  // ?2 ? bit3(????????)
    State |= GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) << 2;  // ?   ? bit2
    State |= GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) << 1;  // ?2 ? bit1
    State |= GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) << 0;  // ?1 ? bit0
    
    return State;
}
