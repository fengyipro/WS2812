#include "ws2812.h"
#include "stm32f1xx_hal.h"

#define WS2812_PIN  GPIO_PIN_13
#define WS2812_PORT GPIOB

// 真正稳定的空指令时序，不抢中断、不影响LCD
void delay_nop(uint32_t cnt)
{
    while(cnt--) __NOP();
}

// 发送 0
void WS2812_Write0(void)
{
    HAL_GPIO_WritePin(WS2812_PORT, WS2812_PIN, 1);
    delay_nop(12);  
    HAL_GPIO_WritePin(WS2812_PORT, WS2812_PIN, 0);
    delay_nop(32);
}

// 发送 1
void WS2812_Write1(void)
{
    HAL_GPIO_WritePin(WS2812_PORT, WS2812_PIN, 1);
    delay_nop(32);
    HAL_GPIO_WritePin(WS2812_PORT, WS2812_PIN, 0);
    delay_nop(12);
}

// 发送一个字节
void WS2812_WriteByte(uint8_t byte)
{
    for(int i=7; i>=0; i--)
    {
        if(byte & (1<<i)) WS2812_Write1();
        else WS2812_Write0();
    }
}

// 颜色顺序 R G B
void WS2812_SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    WS2812_WriteByte(g);
    WS2812_WriteByte(r);
    WS2812_WriteByte(b);
}

// 刷新
void WS2812_Show(void)
{
    HAL_GPIO_WritePin(WS2812_PORT, WS2812_PIN, 0);
    delay_nop(150);
}