#include "micphone.h"

/**
 * @brief 声音传感器初始化
 */
void Micphone_Init(void) {
    // ADC1已经在mx_adc1_init中初始化
    HAL_ADCEx_Calibration_Start(&hadc1); // 启动校准
}

/**
 * @brief 获取声音强度 (单次采样)
 */
uint32_t Micphone_GetValue(void) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        return HAL_ADC_GetValue(&hadc1);
    }
    return 0;
}

/**
 * @brief 获取平滑后的声音强度
 */
uint32_t Micphone_GetAverage(uint16_t samples) {
    uint32_t sum = 0;
    for (uint16_t i = 0; i < samples; i++) {
        sum += Micphone_GetValue();
    }
    return sum / samples;
}
