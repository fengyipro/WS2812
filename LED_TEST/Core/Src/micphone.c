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
 * @brief 初始化麦克风校准
 */
void Mic_Calibration_Init(Mic_Calibration_t *cal) {
    cal->state = MIC_CALIBRATING;
    cal->start_time = HAL_GetTick();
    cal->sum = 0;
    cal->count = 0;
    cal->noise_threshold = 0;
}

/**
 * @brief 麦克风校准处理
 *
 * 校准过程：
 * 1. 采集5秒的环境噪音数据 (每100ms采样一次，共50次)
 * 2. 计算5秒内的ADC均值
 * 3. 将均值乘以1.5作为噪音阈值
 *
 * @param cal 校准结构体指针
 * @param mic_val 当前ADC采样值
 * @return 0=校准中, 1=校准完成
 */
uint8_t Mic_Calibration_Process(Mic_Calibration_t *cal, uint32_t mic_val) {
    if (cal->state == MIC_CALIBRATED) {
        return 1;
    }

    // 累加采样值
    cal->sum += mic_val;
    cal->count++;

    // 检查是否已经采集了5秒 (每100ms一次，共50次)
    uint32_t elapsed = HAL_GetTick() - cal->start_time;
    if (elapsed >= 5000 || cal->count >= 50) {
        // 计算均值
        uint32_t average = cal->sum / cal->count;

        // 阈值 = 均值 * 1.5 (使用定点数计算，放大10倍后除以10)
        cal->noise_threshold = (average * 15) / 10;

        cal->state = MIC_CALIBRATED;
        return 1;
    }

    return 0;
}
