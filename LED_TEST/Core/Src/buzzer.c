#include "buzzer.h"

static uint8_t buzzer_active = 0;
static uint32_t buzzer_off_tick = 0;

/**
 * @brief 蜂鸣器初始化
 * 注意：GPIO配置已在mx_gpio_init中完成，这里仅确保初始状态为关闭
 */
void Buzzer_Init(void) {
    BEEP_OFF();
    buzzer_active = 0;
    buzzer_off_tick = 0;
}

/**
 * @brief 蜂鸣器短鸣反馈（非阻塞）
 */
void Buzzer_Beep_Short(void) {
    BEEP_ON();
    buzzer_active = 1;
    buzzer_off_tick = HAL_GetTick() + 15U;
}

/**
 * @brief 蜂鸣器周期更新（非阻塞短鸣定时）
 */
void Buzzer_Update(void) {
    if (!buzzer_active) {
        return;
    }

    if ((int32_t)(HAL_GetTick() - buzzer_off_tick) >= 0) {
        BEEP_OFF();
        buzzer_active = 0;
    }
}
