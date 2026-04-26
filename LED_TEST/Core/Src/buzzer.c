#include "buzzer.h"

/**
 * @brief 蜂鸣器初始化
 * 注意：GPIO配置已在mx_gpio_init中完成，这里仅确保初始状态为关闭
 */
void Buzzer_Init(void) {
    BEEP_OFF();
}

/**
 * @brief 蜂鸣器短鸣反馈 (非阻塞式简单延时，由于是按键触发，短时间延时可接受)
 */
void Buzzer_Beep_Short(void) {
    BEEP_ON();
    HAL_Delay(50); // 50ms 短鸣
    BEEP_OFF();
}