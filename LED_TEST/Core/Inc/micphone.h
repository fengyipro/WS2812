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
 * @brief 麦克风校准状态枚举
 */
typedef enum {
    MIC_CALIBRATING,    // 正在校准中
    MIC_CALIBRATED      // 校准完成
} Mic_CalibState_t;

/**
 * @brief 麦克风校准结构体
 */
typedef struct {
    Mic_CalibState_t state;     // 校准状态
    uint32_t start_time;        // 校准开始时间
    uint32_t sum;               // 采样值累加
    uint32_t count;             // 采样次数
    uint32_t noise_threshold;   // 计算出的噪音阈值
} Mic_Calibration_t;

/**
 * @brief 初始化麦克风校准
 * @param cal 校准结构体指针
 */
void Mic_Calibration_Init(Mic_Calibration_t *cal);

/**
 * @brief 麦克风校准处理 (每100ms调用一次)
 * @param cal 校准结构体指针
 * @param mic_val 当前ADC采样值
 * @return 0=校准中, 1=校准完成
 */
uint8_t Mic_Calibration_Process(Mic_Calibration_t *cal, uint32_t mic_val);

#endif /* __MICPHONE_H */
