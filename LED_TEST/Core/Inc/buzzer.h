#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

// 蜂鸣器引脚定义 (PB12)
#define BEEP_PIN        GPIO_PIN_12
#define BEEP_PORT       GPIOB

// 蜂鸣器控制宏 (低电平触发)
#define BEEP_ON()       HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_RESET)
#define BEEP_OFF()      HAL_GPIO_WritePin(BEEP_PORT, BEEP_PIN, GPIO_PIN_SET)
#define BEEP_TOGGLE()   HAL_GPIO_TogglePin(BEEP_PORT, BEEP_PIN)

/**
 * @brief 蜂鸣器初始化
 */
void Buzzer_Init(void);

/**
 * @brief 蜂鸣器短鸣反馈
 */
void Buzzer_Beep_Short(void);

/**
 * @brief 蜂鸣器周期更新（非阻塞短鸣定时）
 */
void Buzzer_Update(void);

#endif /* __BUZZER_H */
