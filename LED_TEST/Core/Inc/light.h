#ifndef __LIGHT_H
#define __LIGHT_H

#include "main.h"

// 光敏电阻模块连接在 PB0 (数字输入)
#define LIGHT_PORT GPIOB
#define LIGHT_PIN  GPIO_PIN_0

/**
 * @brief 光敏传感器初始化
 */
void Light_Init(void);

/**
 * @brief 检测环境是否明亮
 * @return 1:明亮, 0:暗
 */
uint8_t Light_IsBright(void);

#endif /* __LIGHT_H */
