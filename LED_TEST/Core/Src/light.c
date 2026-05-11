#include "light.h"

/**
 * @brief 光敏传感器初始化
 *        已经在 gpio.c 的 MX_GPIO_Init() 中完成输入上拉配置
 */
void Light_Init(void) {
    // 留空，CubeMX已配置 PB0 为输入
}

/**
 * @brief 检测环境是否明亮
 * @return 1:明亮, 0:暗
 */
uint8_t Light_IsBright(void) {
    // 光敏模块通常：光照强时 DO 输出低电平(1)，光照弱时输出高电平(0)
    if (HAL_GPIO_ReadPin(LIGHT_PORT, LIGHT_PIN) == GPIO_PIN_RESET) {
        return 0; // 环境暗
    } else {
        return 1; // 环境明亮
    }
}
