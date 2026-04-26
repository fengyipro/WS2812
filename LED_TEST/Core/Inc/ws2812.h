#ifndef __WS2812_H
#define __WS2812_H

#include "main.h"
#include "tim.h"

// WS2812 参数定义
#define LED_MAX_COUNT   64      // 最大支持的LED数量，可根据需要调整
#define WS2812_T0H      25      // 0码高电平时间 (约0.35us @ 72MHz, Period=90)
#define WS2812_T1H      58      // 1码高电平时间 (约0.8us @ 72MHz, Period=90)
#define WS2812_RESET_LEN 50     // 复位信号长度

typedef struct {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} RGB_Color_t;

/**
 * @brief WS2812 初始化
 */
void WS2812_Init(void);

/**
 * @brief 设置单个LED颜色
 * @param index LED索引 (0开始)
 * @param r, g, b 颜色值 (0-255)
 */
void WS2812_Set_Color(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 更新灯带显示 (DMA发送)
 */
void WS2812_Show(uint16_t num);

/**
 * @brief 清除所有灯
 */
void WS2812_Clear(uint16_t num);

/**
 * @brief 设置全亮颜色
 */
void WS2812_Set_All(uint16_t num, uint8_t r, uint8_t g, uint8_t b);

#endif /* __WS2812_H */
