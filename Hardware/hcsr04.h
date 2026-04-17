#ifndef __HCSR04_H
#define __HCSR04_H

#include "stm32f10x.h"  // 基础依赖头文件

// 外部声明计时变量（供中断和主函数使用）
extern uint16_t Time;

/************************ 函数声明 ************************/
/**
 * @brief  初始化HC-SR04模块（GPIO+定时器）
 * @param  无
 * @retval 无
 */
void HCSR04_Init(void);

/**
 * @brief  启动一次超声波测距（发送触发信号）
 * @param  无
 * @retval 无
 */
void HCSR04_Start(void);

/**
 * @brief  获取测距结果（单位：cm）
 * @param  无
 * @retval 距离值 0~400
 */
uint16_t HCSR04_GetValue(void);

#endif
