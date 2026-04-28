/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "ws2812.h"
#include "micphone.h"
#include "buzzer.h"
#include "light.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    MENU_HOME,
    MENU_BRIGHTNESS,
    MENU_EXTENDED,
    MENU_COLOR,
    MENU_MUSIC,
    MENU_AUTO_BRIGHTNESS,
    MENU_ANIMATION
} MenuState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_NUM 64
#define MATRIX_W 8
#define MATRIX_H 8
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile MenuState_t current_menu = MENU_HOME;
volatile uint8_t key1_pressed = 0;
volatile uint8_t key2_pressed = 0;

uint8_t brightness_level = 2; // 1:Dark, 2:Medium, 3:Bright
uint8_t color_index = 0;
uint8_t extended_menu_cursor = 0; // 0:Color, 1:Music, 2:Auto Brightness, 3:Animation
uint8_t animation_index = 0;      // 0~2 三种动画
RGB_Color_t colors[] = {
    {255, 0, 0},   // Red
    {0, 255, 0},   // Green
    {0, 0, 255},   // Blue
    {255, 255, 0}, // Yellow
    {255, 0, 255}, // Purple
    {0, 255, 255}, // Cyan
    {255, 255, 255} // White
};
#define COLOR_COUNT (sizeof(colors)/sizeof(colors[0]))

uint32_t last_key_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void UI_Refresh(void);
void Handle_Keys(void);
void Music_Rhythm_Update(void);
void Auto_Brightness_Update(void);
void Animation_Update(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint16_t Matrix_Index(uint8_t x, uint8_t y) {
    // 按常见蛇形走线映射: 偶数行左->右, 奇数行右->左
    if (y & 0x01U) {
        return (uint16_t)(y * MATRIX_W + (MATRIX_W - 1U - x));
    }
    return (uint16_t)(y * MATRIX_W + x);
}

static void Matrix_SetPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= MATRIX_W || y >= MATRIX_H) {
        return;
    }
    WS2812_Set_Color(Matrix_Index(x, y), r, g, b);
}

static void HSV_To_RGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region;
    uint16_t remainder;
    uint8_t p, q, t;

    if (s == 0U) {
        *r = v; *g = v; *b = v;
        return;
    }

    h %= 360U;
    region = (uint8_t)(h / 60U);
    remainder = (uint16_t)((h % 60U) * 255U / 60U);

    p = (uint8_t)((uint16_t)v * (255U - s) / 255U);
    q = (uint8_t)((uint16_t)v * (255U - ((uint16_t)s * remainder / 255U)) / 255U);
    t = (uint8_t)((uint16_t)v * (255U - ((uint16_t)s * (255U - remainder) / 255U)) / 255U);

    switch (region) {
        case 0: *r = v; *g = t; *b = p; break;
        case 1: *r = q; *g = v; *b = p; break;
        case 2: *r = p; *g = v; *b = t; break;
        case 3: *r = p; *g = q; *b = v; break;
        case 4: *r = t; *g = p; *b = v; break;
        default:*r = v; *g = p; *b = q; break;
    }
}

static uint8_t Fast_Rand8(void) {
    static uint32_t rng_state = 0x12345678UL;
    rng_state = rng_state * 1664525UL + 1013904223UL;
    return (uint8_t)(rng_state >> 24);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();
  LCD_Clear(BLACK);
  WS2812_Init();
  Micphone_Init();
  Buzzer_Init();
  Light_Init();
  
  UI_Refresh(); // 显示初始界面
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Handle_Keys();
    
    if (current_menu == MENU_MUSIC) {
        Music_Rhythm_Update();
    } else if (current_menu == MENU_AUTO_BRIGHTNESS) {
        Auto_Brightness_Update();
    } else if (current_menu == MENU_ANIMATION) {
        Animation_Update();
    }
    
    HAL_Delay(10); // 适当延时，减轻CPU负担
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief 外部中断回调函数，处理按键触发
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t now = HAL_GetTick();
    if (now - last_key_time < 200) { // 200ms 软件消抖
        return;
    }
    
    if (GPIO_Pin == KEY_1_Pin) {
        key1_pressed = 1;
        last_key_time = now;
    } else if (GPIO_Pin == KEY_2_Pin) {
        key2_pressed = 1;
        last_key_time = now;
    }
}

/**
 * @brief 处理按键逻辑处理与状态跳转
 */
void Handle_Keys(void) {
    uint8_t val;

    if (key1_pressed) {
        key1_pressed = 0;
        Buzzer_Beep_Short(); // 在主循环中安全调用
        switch (current_menu) {
            case MENU_HOME:
                current_menu = MENU_BRIGHTNESS;
                UI_Refresh();
                break;
            case MENU_BRIGHTNESS:
                if (brightness_level < 3) brightness_level++;
                else brightness_level = 1; // 循环切换
                UI_Refresh();
                // 调节WS2812亮度
                val = (brightness_level == 1) ? 30 : (brightness_level == 2 ? 120 : 255);
                WS2812_Set_All(LED_NUM, val, val, val);
                break;
            case MENU_EXTENDED:
                // K1 循环切换拓展功能
                extended_menu_cursor = (extended_menu_cursor + 1) % 4;
                UI_Refresh();
                break;
            case MENU_COLOR:
                color_index = (color_index + 1) % COLOR_COUNT;
                WS2812_Set_All(LED_NUM, colors[color_index].R, colors[color_index].G, colors[color_index].B);
                UI_Refresh();
                break;
            case MENU_MUSIC:
            case MENU_AUTO_BRIGHTNESS:
                // 在这些模式下，按键1退出到扩展菜单
                current_menu = MENU_EXTENDED;
                WS2812_Clear(LED_NUM);
                UI_Refresh();
                break;
            case MENU_ANIMATION:
                // 动画模式内切换动画类型
                animation_index = (animation_index + 1) % 3;
                UI_Refresh();
                break;
        }
    }
    
    if (key2_pressed) {
        key2_pressed = 0;
        Buzzer_Beep_Short(); // 在主循环中安全调用
        switch (current_menu) {
            case MENU_HOME:
                current_menu = MENU_EXTENDED;
                UI_Refresh();
                break;
            case MENU_BRIGHTNESS:
                if (brightness_level > 1) brightness_level--;
                else brightness_level = 3; // 循环切换
                UI_Refresh();
                val = (brightness_level == 1) ? 30 : (brightness_level == 2 ? 120 : 255);
                WS2812_Set_All(LED_NUM, val, val, val);
                break;
            case MENU_EXTENDED:
                // K2 确认进入模式
                if (extended_menu_cursor == 0) {
                    current_menu = MENU_COLOR;
                } else if (extended_menu_cursor == 1) {
                    current_menu = MENU_MUSIC;
                } else if (extended_menu_cursor == 2) {
                    current_menu = MENU_AUTO_BRIGHTNESS;
                } else {
                    current_menu = MENU_ANIMATION;
                }
                UI_Refresh();
                break;
            case MENU_COLOR:
            case MENU_MUSIC:
            case MENU_AUTO_BRIGHTNESS:
            case MENU_ANIMATION:
                // 按键2退出
                current_menu = MENU_EXTENDED;
                WS2812_Clear(LED_NUM);
                UI_Refresh();
                break;
        }
    }
}

/**
 * @brief 刷新LCD界面显示 (适配1.44寸 128x128 屏幕)
 */
void UI_Refresh(void) {
    char buf[32];
    const char* level_str;
    const char* color_names[] = {"RED", "GREEN", "BLUE", "YELLOW", "PURPLE", "CYAN", "WHITE"};
    const char* anim_names[] = {"Rainbow Flow", "Breath Pulse", "Pinwheel"};
    uint8_t bar_end;

    LCD_Clear(BLACK);
    
    // 绘制顶部标题栏 (简约设计)
    LCD_Fill(0, 0, 127, 20, BLUE);
    
    switch (current_menu) {
        case MENU_HOME:
            LCD_ShowString(32, 2, (u8*)"MAIN MENU", WHITE, BLUE, 16, 0);
            
            LCD_ShowString(10, 45, (u8*)"1. Brightness", WHITE, BLACK, 16, 0);
            LCD_ShowString(10, 75, (u8*)"2. Extended", WHITE, BLACK, 16, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(15, 110, (u8*)"K1:Enter K2:Next", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_BRIGHTNESS:
            LCD_ShowString(24, 2, (u8*)"BRIGHTNESS", WHITE, BLUE, 16, 0);
            
            level_str = (brightness_level == 1) ? "DARK" : (brightness_level == 2 ? "MEDIUM" : "BRIGHT");
            sprintf(buf, "Level: %s", level_str);
            LCD_ShowString(20, 50, (u8*)buf, WHITE, BLACK, 16, 0);
            
            // 简单的进度条
            LCD_DrawLine(20, 80, 108, 80, WHITE);
            bar_end = (uint8_t)(20 + (brightness_level * 29));
            LCD_Fill(20, 77, bar_end, 83, CYAN);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:+  K2:-  K1+K2:Exit", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_EXTENDED:
            LCD_ShowString(28, 2, (u8*)"EXTENDED", WHITE, BLUE, 16, 0);
            
            LCD_ShowString(8, 30, (u8*)"1. Color Mode", (extended_menu_cursor == 0) ? CYAN : WHITE, BLACK, 12, 0);
            LCD_ShowString(8, 48, (u8*)"2. Music Matrix", (extended_menu_cursor == 1) ? CYAN : WHITE, BLACK, 12, 0);
            LCD_ShowString(8, 66, (u8*)"3. Auto Bright", (extended_menu_cursor == 2) ? CYAN : WHITE, BLACK, 12, 0);
            LCD_ShowString(8, 84, (u8*)"4. Animation", (extended_menu_cursor == 3) ? CYAN : WHITE, BLACK, 12, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:Sel  K2:Enter", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_COLOR:
            LCD_ShowString(24, 2, (u8*)"COLOR MODE", WHITE, BLUE, 16, 0);
            
            sprintf(buf, "Index: %d", color_index);
            LCD_ShowString(30, 50, (u8*)buf, WHITE, BLACK, 16, 0);
            
            // 显示颜色名称
            sprintf(buf, "Color: %s", color_names[color_index]);
            LCD_ShowString(20, 80, (u8*)buf, CYAN, BLACK, 16, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:Next  K2:Back", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_MUSIC:
            LCD_ShowString(12, 2, (u8*)"MUSIC MATRIX", WHITE, BLUE, 16, 0);
            
            LCD_ShowString(8, 42, (u8*)"8x8 Gradient Beat", WHITE, BLACK, 12, 0);
            // 进度条边框
            LCD_DrawLine(14, 75, 114, 75, GRAY);
            LCD_DrawLine(14, 85, 114, 85, GRAY);
            LCD_DrawLine(14, 75, 14, 85, GRAY);
            LCD_DrawLine(114, 75, 114, 85, GRAY);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(25, 110, (u8*)"K1 / K2: Exit", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_AUTO_BRIGHTNESS:
            LCD_ShowString(10, 2, (u8*)"AUTO BRIGHTNESS", WHITE, BLUE, 16, 0);
            
            LCD_ShowString(20, 60, (u8*)"Sensing Light...", CYAN, BLACK, 16, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(25, 110, (u8*)"K1 / K2: Exit", GRAY, BLACK, 12, 0);
            break;

        case MENU_ANIMATION:
            LCD_ShowString(16, 2, (u8*)"ANIMATION", WHITE, BLUE, 16, 0);
            sprintf(buf, "Mode: %d/3", animation_index + 1);
            LCD_ShowString(20, 48, (u8*)buf, WHITE, BLACK, 16, 0);
            LCD_ShowString(8, 72, (u8*)anim_names[animation_index], CYAN, BLACK, 12, 0);
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(4, 110, (u8*)"K1:Change  K2:Back", GRAY, BLACK, 12, 0);
            break;
    }
}

/**
 * @brief 音乐律动逻辑更新
 */
void Music_Rhythm_Update(void) {
    static uint32_t last_rhythm_time = 0;
    static uint8_t last_bar_length = 0;
    static uint32_t last_beat_time = 0;
    static uint8_t tide_dir = 0;
    static uint16_t tide_phase = 0;
    static uint8_t tide_energy = 0;
    static uint8_t activity_avg = 0;

    uint8_t activity;
    uint8_t bar_length;
    uint32_t now = HAL_GetTick();
    uint8_t x, y;
    
    if (now - last_rhythm_time < 30) return;
    last_rhythm_time = now;

    activity = Micphone_GetActivityLevel();
    activity_avg = (uint8_t)(((uint16_t)activity_avg * 7U + activity) / 8U);
    bar_length = (uint8_t)(((uint16_t)activity * 98U) / 255U);

    // 节拍触发：相对均值抬升 + 最小触发间隔，保证跟随外部音乐节奏
    if (activity > (uint8_t)(activity_avg + 24U) && (now - last_beat_time) > 140U) {
        tide_dir = Fast_Rand8() & 0x07U;   // 8个方向随机
        tide_phase = 0;
        tide_energy = activity;
        last_beat_time = now;
    }

    if (tide_energy > 2U) {
        tide_energy -= 2U; // 慢衰减，形成潮汐余波
    } else {
        tide_energy = 0;
    }

    tide_phase = (uint16_t)(tide_phase + 14U + (tide_energy >> 4)); // 速度随能量变化

    for (y = 0; y < MATRIX_H; y++) {
        for (x = 0; x < MATRIX_W; x++) {
            uint8_t dist;
            uint16_t dist_q;
            uint16_t delta;
            uint16_t crest;
            uint8_t val;
            uint16_t hue;
            uint8_t r, g, b;

            // 根据方向计算到边界的距离(0~7)，形成潮汐波前
            switch (tide_dir) {
                case 0: dist = y; break;                         // 上->下
                case 1: dist = (uint8_t)(7U - y); break;         // 下->上
                case 2: dist = x; break;                         // 左->右
                case 3: dist = (uint8_t)(7U - x); break;         // 右->左
                case 4: dist = (uint8_t)((x + y) >> 1); break;   // 左上->右下
                case 5: dist = (uint8_t)((14U - x - y) >> 1); break; // 右下->左上
                case 6: dist = (uint8_t)(((7U - x) + y) >> 1); break; // 右上->左下
                default:dist = (uint8_t)((x + (7U - y)) >> 1); break; // 左下->右上
            }

            dist_q = (uint16_t)(dist * 32U);
            delta = (dist_q > tide_phase) ? (dist_q - tide_phase) : (tide_phase - dist_q);

            // 仅波峰附近点亮，避免全屏同亮
            if (delta < 52U) {
                crest = (uint16_t)(52U - delta);
                val = (uint8_t)(((uint32_t)crest * (uint16_t)(70U + tide_energy)) / 52U);
            } else {
                val = 0;
            }

            // 增加稀疏纹理，让潮汐更像“水波”而不是整行亮
            if (val > 0U && (((uint8_t)(x * 13U + y * 7U + (now >> 4)) & 0x03U) == 0U)) {
                val = (uint8_t)(val >> 1);
            }

            if (val < 8U) {
                Matrix_SetPixel(x, y, 0, 0, 0);
                continue;
            }

            hue = (uint16_t)((now / 10U + tide_dir * 33U + dist * 20U) % 360U);
            HSV_To_RGB(hue, 220U, val, &r, &g, &b);
            Matrix_SetPixel(x, y, r, g, b);
        }
    }
    WS2812_Show(LED_NUM);
    
    // 更新LCD进度条显示 (15 到 113)
    if (current_menu == MENU_MUSIC && bar_length != last_bar_length) {
        if (bar_length > last_bar_length) {
            LCD_Fill(15 + last_bar_length, 76, 15 + bar_length - 1, 84, CYAN);
        } else {
            LCD_Fill(15 + bar_length, 76, 15 + last_bar_length - 1, 84, BLACK);
        }
        last_bar_length = bar_length;
    }
}

/**
 * @brief 8x8矩阵动画更新
 */
void Animation_Update(void) {
    static uint32_t last_anim_time = 0;
    uint32_t now = HAL_GetTick();
    uint8_t x, y;

    if (now - last_anim_time < 60) {
        return;
    }
    last_anim_time = now;

    for (y = 0; y < MATRIX_H; y++) {
        for (x = 0; x < MATRIX_W; x++) {
            uint8_t on = 0;
            uint16_t hue = 0;
            uint8_t val;
            uint8_t r, g, b;

            if (animation_index == 0) {
                // 动画1: 边框流星（仅边缘少量灯珠 + 拖尾）
                uint8_t step = (uint8_t)((now / 70U) % 28U);
                uint8_t tail;
                uint8_t px = 0, py = 0;

                if (step < 8U) { px = step; py = 0; }
                else if (step < 14U) { px = 7U; py = (uint8_t)(step - 7U); }
                else if (step < 22U) { px = (uint8_t)(21U - step); py = 7U; }
                else { px = 0; py = (uint8_t)(28U - step); }

                on = (x == px && y == py);
                for (tail = 1; tail <= 3U && !on; tail++) {
                    uint8_t ts = (uint8_t)((step + 28U - tail) % 28U);
                    uint8_t tx = 0, ty = 0;
                    if (ts < 8U) { tx = ts; ty = 0; }
                    else if (ts < 14U) { tx = 7U; ty = (uint8_t)(ts - 7U); }
                    else if (ts < 22U) { tx = (uint8_t)(21U - ts); ty = 7U; }
                    else { tx = 0; ty = (uint8_t)(28U - ts); }
                    if (x == tx && y == ty) on = 1;
                }

                hue = (uint16_t)((now / 8U + x * 10U + y * 6U) % 360U);
                val = on ? ((x == px && y == py) ? 220U : 90U) : 0U;
            } else if (animation_index == 1) {
                // 动画2: 十字呼吸（中心十字脉冲，非全屏）
                uint16_t phase = (uint16_t)((now / 9U) & 0x3FU);
                uint8_t tri = (phase < 32U) ? (uint8_t)phase : (uint8_t)(63U - phase);
                uint8_t arm = (uint8_t)(1U + (tri / 10U)); // 1~4
                int8_t dx = (int8_t)x - 3;
                int8_t dy = (int8_t)y - 3;
                int8_t adx = (dx >= 0) ? dx : (int8_t)(-dx);
                int8_t ady = (dy >= 0) ? dy : (int8_t)(-dy);

                if ((dx == 0 && ady <= (int8_t)arm) || (dy == 0 && adx <= (int8_t)arm)) {
                    on = 1;
                }

                hue = (uint16_t)((now / 12U + tri * 4U) % 360U);
                val = on ? (uint8_t)(60U + (uint16_t)tri * 5U) : 0U;
            } else {
                // 动画3: 星点闪烁（稀疏随机点，颜色渐变）
                uint8_t seed = (uint8_t)(x * 19U + y * 37U + (now >> 5));
                if ((seed & 0x0FU) == 0U || (seed & 0x1FU) == 3U) {
                    on = 1;
                }

                hue = (uint16_t)((now / 7U + x * 22U + y * 14U) % 360U);
                val = on ? (uint8_t)(90U + (seed & 0x3FU)) : 0U;
            }

            if (val > 0U) {
                HSV_To_RGB(hue, 220U, val, &r, &g, &b);
                Matrix_SetPixel(x, y, r, g, b);
            } else {
                Matrix_SetPixel(x, y, 0, 0, 0);
            }
        }
    }

    WS2812_Show(LED_NUM);
}

/**
 * @brief 自动亮度更新
 */
void Auto_Brightness_Update(void) {
    static uint32_t last_check_time = 0;
    uint32_t now = HAL_GetTick();
    
    if (now - last_check_time < 500) return; // 500ms检查一次
    last_check_time = now;
    
    if (Light_IsBright()) {
        WS2812_Set_All(LED_NUM, 255, 255, 255); // 明亮环境：高亮度
    } else {
        WS2812_Set_All(LED_NUM, 30, 30, 30);    // 昏暗环境：低亮度
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
