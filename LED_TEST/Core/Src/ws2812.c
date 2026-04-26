#include "ws2812.h"
#include <string.h>

// PWM数据缓存：每个LED占24位 (G, R, B) + 复位信号
static uint16_t ws2812_pixel_data[LED_MAX_COUNT * 24 + WS2812_RESET_LEN];

/**
 * @brief WS2812 初始化
 */
void WS2812_Init(void) {
    memset(ws2812_pixel_data, 0, sizeof(ws2812_pixel_data));
}

/**
 * @brief 设置单个LED颜色 (GRB格式)
 */
void WS2812_Set_Color(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= LED_MAX_COUNT) return;

    for (int i = 0; i < 8; i++) {
        // WS2812 顺序是 G7..G0, R7..R0, B7..B0
        ws2812_pixel_data[index * 24 + i]      = (g & (0x80 >> i)) ? WS2812_T1H : WS2812_T0H;
        ws2812_pixel_data[index * 24 + 8 + i]  = (r & (0x80 >> i)) ? WS2812_T1H : WS2812_T0H;
        ws2812_pixel_data[index * 24 + 16 + i] = (b & (0x80 >> i)) ? WS2812_T1H : WS2812_T0H;
    }
}

/**
 * @brief 更新灯带显示
 */
void WS2812_Show(uint16_t num) {
    if (num > LED_MAX_COUNT) num = LED_MAX_COUNT;
    
    // 确保复位信号部分为0
    for (int i = 0; i < WS2812_RESET_LEN; i++) {
        ws2812_pixel_data[num * 24 + i] = 0;
    }

    // 启动 DMA 发送 (注意使用的是 TIM1_CH1N)
    HAL_TIMEx_PWMN_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)ws2812_pixel_data, num * 24 + WS2812_RESET_LEN);
}

/**
 * @brief 清除所有灯
 */
void WS2812_Clear(uint16_t num) {
    for (uint16_t i = 0; i < num; i++) {
        WS2812_Set_Color(i, 0, 0, 0);
    }
    WS2812_Show(num);
}

/**
 * @brief 设置全亮颜色
 */
void WS2812_Set_All(uint16_t num, uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < num; i++) {
        WS2812_Set_Color(i, r, g, b);
    }
    WS2812_Show(num);
}

/**
 * @brief DMA 传输完成回调 (停止 PWM 防止干扰)
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        HAL_TIMEx_PWMN_Stop_DMA(&htim1, TIM_CHANNEL_1);
    }
}
