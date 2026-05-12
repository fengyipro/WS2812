#ifndef __LIGHT_H
#define __LIGHT_H

#include "main.h"
#include "adc.h"

#define LIGHT_ADC_CHANNEL ADC_CHANNEL_8

/**
 * @brief 光敏传感器初始化
 */
void Light_Init(void);

/**
 * @brief 获取光照强度ADC原始值
 * @return 0-4095
 */
uint32_t Light_GetValue(void);

uint32_t Light_GetAverage(uint16_t samples);

#endif /* __LIGHT_H */
