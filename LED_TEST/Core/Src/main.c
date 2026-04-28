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
    MENU_AUTO_BRIGHTNESS
} MenuState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_NUM 64 // 有10个LED，驱动库是兼容的
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
uint8_t extended_menu_cursor = 0; // 0:Color, 1:Music, 2:Auto Brightness
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
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
                uint8_t val = (brightness_level == 1) ? 30 : (brightness_level == 2 ? 120 : 255);
                WS2812_Set_All(LED_NUM, val, val, val);
                break;
            case MENU_EXTENDED:
                // K1 循环切换拓展功能
                extended_menu_cursor = (extended_menu_cursor + 1) % 3;
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
                uint8_t val_dec = (brightness_level == 1) ? 30 : (brightness_level == 2 ? 120 : 255);
                WS2812_Set_All(LED_NUM, val_dec, val_dec, val_dec);
                break;
            case MENU_EXTENDED:
                // K2 确认进入模式
                if (extended_menu_cursor == 0) {
                    current_menu = MENU_COLOR;
                } else if (extended_menu_cursor == 1) {
                    current_menu = MENU_MUSIC;
                } else {
                    current_menu = MENU_AUTO_BRIGHTNESS;
                }
                UI_Refresh();
                break;
            case MENU_COLOR:
            case MENU_MUSIC:
            case MENU_AUTO_BRIGHTNESS:
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
            
            const char* level_str = (brightness_level == 1) ? "DARK" : (brightness_level == 2 ? "MEDIUM" : "BRIGHT");
            sprintf(buf, "Level: %s", level_str);
            LCD_ShowString(20, 50, (u8*)buf, WHITE, BLACK, 16, 0);
            
            // 简单的进度条
            LCD_DrawLine(20, 80, 108, 80, WHITE);
            uint8_t bar_end = 20 + (brightness_level * 29);
            LCD_Fill(20, 77, bar_end, 83, CYAN);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:+  K2:-  K1+K2:Exit", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_EXTENDED:
            LCD_ShowString(28, 2, (u8*)"EXTENDED", WHITE, BLUE, 16, 0);
            
            LCD_ShowString(10, 35, (u8*)"1. Color Mode", (extended_menu_cursor == 0) ? CYAN : WHITE, BLACK, 16, 0);
            LCD_ShowString(10, 60, (u8*)"2. Music Mode", (extended_menu_cursor == 1) ? CYAN : WHITE, BLACK, 16, 0);
            LCD_ShowString(10, 85, (u8*)"3. Auto Bright", (extended_menu_cursor == 2) ? CYAN : WHITE, BLACK, 16, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:Sel  K2:Enter", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_COLOR:
            LCD_ShowString(24, 2, (u8*)"COLOR MODE", WHITE, BLUE, 16, 0);
            
            sprintf(buf, "Index: %d", color_index);
            LCD_ShowString(30, 50, (u8*)buf, WHITE, BLACK, 16, 0);
            
            // 显示颜色名称
            const char* color_names[] = {"RED", "GREEN", "BLUE", "YELLOW", "PURPLE", "CYAN", "WHITE"};
            sprintf(buf, "Color: %s", color_names[color_index]);
            LCD_ShowString(20, 80, (u8*)buf, CYAN, BLACK, 16, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:Next  K2:Back", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_MUSIC:
            LCD_ShowString(16, 2, (u8*)"MUSIC RHYTHM", WHITE, BLUE, 16, 0);
            
            LCD_ShowString(20, 45, (u8*)"Listening...", WHITE, BLACK, 16, 0);
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
    }
}

/**
 * @brief 音乐律动逻辑更新
 */
void Music_Rhythm_Update(void) {
    static uint32_t last_rhythm_time = 0;
    static uint8_t last_bar_length = 0;
    uint32_t now = HAL_GetTick();
    
    if (now - last_rhythm_time < 50) return; // 50ms采样一次
    last_rhythm_time = now;
    
    uint32_t mic_val = Micphone_GetAverage(10);
    
    // 简单的律动逻辑：根据声音强度映射到亮度和颜色
    // 静默环境 ADC 值在 2600 左右 (需根据实际环境调整)
    int threshold = 2000;
    uint8_t bar_length = 0;
    
    if (mic_val > threshold) {
        uint32_t diff = mic_val - threshold;
        uint8_t intensity = diff / 10;
        if (intensity > 255) intensity = 255;
        
        // 随节拍变换颜色 (简单模拟：强度越高颜色越偏红)
        WS2812_Set_All(LED_NUM, intensity, 255 - intensity, intensity / 2);
        
        // 计算进度条长度 (最大长度 98)
        bar_length = diff / 20; 
        if (bar_length > 98) bar_length = 98;
    } else {
        WS2812_Set_All(LED_NUM, 10, 10, 10); // 微弱亮
        bar_length = 0;
    }
    
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
