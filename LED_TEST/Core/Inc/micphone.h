#ifndef __MICPHONE_H
#define __MICPHONE_H

#include "main.h"
#include "adc.h"

/**
 * @brief 声音传感器初始化
 */
void Micphone_Init(void);

/**
 * @brief 获取声音强度 (原始ADC值)
 * @return uint32_t 0-4095
 */
uint32_t Micphone_GetValue(void);

/**
 * @brief 获取平滑后的声音强度 (简单的均值滤波)
 * @return uint32_t
 */
uint32_t Micphone_GetAverage(uint16_t samples);

/**
 * @brief 获取0~255的自适应声音活动强度
 * @note  内部会动态跟踪环境噪声底，兼顾灵敏度和抗噪能力
 */
uint8_t Micphone_GetActivityLevel(void);

#endif /* __MICPHONE_H */
