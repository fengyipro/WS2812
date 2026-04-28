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

/**
 * @brief 获取0~255的自适应声音活动强度
 * @note  逻辑:
 *        1) 动态基线 baseline 跟踪环境直流分量
 *        2) 噪声估计 noise_floor 跟踪环境抖动强度
 *        3) 峰值包络 envelope + 衰减，输出稳定且灵敏的活动值
 */
uint8_t Micphone_GetActivityLevel(void) {
    static uint16_t baseline = 2048;
    static uint16_t noise_floor = 40;
    static uint16_t envelope = 0;
    static uint8_t initialized = 0;

    uint32_t raw = Micphone_GetAverage(4);
    uint16_t sample = (uint16_t)raw;

    if (!initialized) {
        baseline = sample;
        initialized = 1;
    }

    // baseline慢速跟踪，避免把短时音乐峰值吞掉
    baseline = (uint16_t)((baseline * 15 + sample) / 16);

    uint16_t deviation = (sample >= baseline) ? (sample - baseline) : (baseline - sample);

    // 噪声底缓慢更新，并给最小安全值，避免过敏
    noise_floor = (uint16_t)((noise_floor * 31 + deviation) / 32);
    if (noise_floor < 10) {
        noise_floor = 10;
    }

    uint16_t gate = (uint16_t)(noise_floor + 6);
    uint16_t signal = (deviation > gate) ? (deviation - gate) : 0;

    // 峰值包络：快起慢落，视觉更跟手
    if (signal > envelope) {
        envelope = signal;
    } else if (envelope > 2) {
        envelope -= 2;
    } else {
        envelope = 0;
    }

    // 归一化映射到0~255，动态范围跟随噪声变化
    {
        uint16_t dynamic_scale = (uint16_t)(noise_floor * 6 + 80);
        uint32_t level = ((uint32_t)envelope * 255U) / dynamic_scale;
        if (level > 255U) {
            level = 255U;
        }
        return (uint8_t)level;
    }
}
