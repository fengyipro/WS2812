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

#endif /* __MICPHONE_H */
