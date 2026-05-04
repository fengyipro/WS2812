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
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    MENU_HOME,
    MENU_BRIGHTNESS,
    MENU_VOICE,
    MENU_EXTENDED,
    MENU_COLOR,
    MENU_MUSIC,
    MENU_AUTO_BRIGHTNESS,
    MENU_SOUND_DETECT    // 声音检测模式
} MenuState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_NUM LED_MAX_COUNT // 统一使用驱动层定义，避免数量不一致
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
uint8_t home_menu_cursor = 0; // 0:Brightness, 1:Extended, 2:Voice
uint8_t extended_menu_cursor = 0; // 0:Color, 1:Music, 2:Auto Brightness, 3:Sound Detect
typedef struct {
    RGB_Color_t rgb;
    const char *name;
} ColorMode_t;

ColorMode_t color_modes[] = {
    {{255, 0, 0}, "RED"},
    {{0, 255, 0}, "GREEN"},
    {{0, 0, 255}, "BLUE"},
    {{255, 255, 0}, "YELLOW"},
    {{255, 0, 255}, "PURPLE"},
    {{0, 255, 255}, "CYAN"},
    {{255, 255, 255}, "WHITE"}
};
#define COLOR_COUNT (sizeof(color_modes)/sizeof(color_modes[0]))

uint32_t last_key_time = 0;

// 麦克风校准结构体
Mic_Calibration_t mic_cal;

// 音乐渐变颜色状态 (定点数，放大256倍)
uint16_t music_hue = 0;        // 当前色相 (0-1535，表示完整色环)
uint8_t music_saturation = 255; // 饱和度
uint8_t music_value = 0;       // 亮度
uint8_t music_cal_initialized = 0; // 音乐模式校准是否已初始化

// 语音模式接收缓冲与状态
uint8_t voice_rx_byte = 0;
char voice_cmd_buf[40];
uint8_t voice_cmd_idx = 0;
char voice_rx_debug[24] = "";
uint8_t voice_rx_debug_idx = 0;
uint8_t voice_rx_debug_updated = 0;
uint32_t voice_led_last_toggle = 0;
uint8_t voice_led_blink_state = 0;
uint32_t voice_led_hold_until = 0;
char voice_last_cmd[24] = "None";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void UI_Refresh(void);
void Handle_Keys(void);
void Music_Rhythm_Update(void);
void Auto_Brightness_Update(void);
void Sound_Detect_Update(void);
void Voice_Mode_Update(void);
void Voice_Process_Command(const char *cmd);
void Voice_LED_Set(uint8_t on);
void HSV_to_RGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
void Matrix_Set_Pixel(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Voice_LED_Set(uint8_t on) {
    // 语音识别LED: 低电平点亮, 高电平熄灭
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
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
  Voice_LED_Set(0);
  
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
    } else if (current_menu == MENU_SOUND_DETECT) {
        Sound_Detect_Update();
    } else if (current_menu == MENU_VOICE) {
        Voice_Mode_Update();
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
                // 主界面: K1用于选择
                home_menu_cursor = (home_menu_cursor + 1) % 3;
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
                WS2812_Set_All(LED_NUM,
                               color_modes[color_index].rgb.R,
                               color_modes[color_index].rgb.G,
                               color_modes[color_index].rgb.B);
                UI_Refresh();
                break;
            case MENU_MUSIC:
                // 退出音乐模式，重置校准状态
                music_cal_initialized = 0;
                music_hue = 0;
                music_value = 0;
                current_menu = MENU_EXTENDED;
                WS2812_Clear(LED_NUM);
                UI_Refresh();
                break;
            case MENU_AUTO_BRIGHTNESS:
            case MENU_SOUND_DETECT:
                // 在这些模式下，按键1退出到扩展菜单
                current_menu = MENU_EXTENDED;
                WS2812_Clear(LED_NUM);
                UI_Refresh();
                break;
            case MENU_VOICE:
                // 语音模式下按键1返回主菜单
                current_menu = MENU_HOME;
                voice_cmd_idx = 0;
                voice_led_hold_until = 0;
                voice_led_blink_state = 0;
                Voice_LED_Set(0);
                UI_Refresh();
                break;
        }
    }
    
    if (key2_pressed) {
        key2_pressed = 0;
        Buzzer_Beep_Short(); // 在主循环中安全调用
        switch (current_menu) {
            case MENU_HOME:
                // 主界面: K2确认进入
                if (home_menu_cursor == 0) {
                    current_menu = MENU_BRIGHTNESS;
                } else if (home_menu_cursor == 1) {
                    current_menu = MENU_EXTENDED;
                } else {
                    current_menu = MENU_VOICE;
                    voice_cmd_idx = 0;
                    voice_led_hold_until = 0;
                    voice_led_last_toggle = HAL_GetTick();
                    voice_led_blink_state = 1;
                    Voice_LED_Set(1);
                }
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
                    // 进入颜色模式时，立即设置LED为当前颜色
                    WS2812_Set_All(LED_NUM,
                                   color_modes[color_index].rgb.R,
                                   color_modes[color_index].rgb.G,
                                   color_modes[color_index].rgb.B);
                } else if (extended_menu_cursor == 1) {
                    current_menu = MENU_MUSIC;
                } else if (extended_menu_cursor == 2) {
                    current_menu = MENU_AUTO_BRIGHTNESS;
                } else {
                    current_menu = MENU_SOUND_DETECT;
                }
                UI_Refresh();
                break;
            case MENU_COLOR:
            case MENU_AUTO_BRIGHTNESS:
            case MENU_SOUND_DETECT:
                // 按键2退出
                current_menu = MENU_EXTENDED;
                WS2812_Clear(LED_NUM);
                UI_Refresh();
                break;
            case MENU_MUSIC:
                // 退出音乐模式，重置校准状态
                music_cal_initialized = 0;
                music_hue = 0;
                music_value = 0;
                current_menu = MENU_EXTENDED;
                WS2812_Clear(LED_NUM);
                UI_Refresh();
                break;
            case MENU_VOICE:
                current_menu = MENU_HOME;
                voice_cmd_idx = 0;
                voice_led_hold_until = 0;
                voice_led_blink_state = 0;
                Voice_LED_Set(0);
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
    uint8_t bar_end;
    LCD_Clear(BLACK);
    
    // 绘制顶部标题栏 (简约设计)
    LCD_Fill(0, 0, 127, 20, BLUE);
    
    switch (current_menu) {
        case MENU_HOME:
            LCD_ShowString(32, 2, (u8*)"MAIN MENU", WHITE, BLUE, 16, 0);

            LCD_ShowString(8, 36, (u8*)"1. Brightness", (home_menu_cursor == 0) ? CYAN : WHITE, BLACK, 16, 0);
            LCD_ShowString(8, 58, (u8*)"2. Extended", (home_menu_cursor == 1) ? CYAN : WHITE, BLACK, 16, 0);
            LCD_ShowString(8, 80, (u8*)"3. Voice Ctrl", (home_menu_cursor == 2) ? CYAN : WHITE, BLACK, 16, 0);

            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:Sel  K2:Enter", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_BRIGHTNESS:
            LCD_ShowString(24, 2, (u8*)"BRIGHTNESS", WHITE, BLUE, 16, 0);
            
            level_str = (brightness_level == 1) ? "DARK" : (brightness_level == 2 ? "MEDIUM" : "BRIGHT");
            sprintf(buf, "Level: %s", level_str);
            LCD_ShowString(20, 50, (u8*)buf, WHITE, BLACK, 16, 0);
            
            // 简单的进度条
            LCD_DrawLine(20, 80, 108, 80, WHITE);
            bar_end = 20 + (brightness_level * 29);
            LCD_Fill(20, 77, bar_end, 83, CYAN);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:+  K2:-  K1+K2:Exit", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_EXTENDED:
            LCD_ShowString(28, 2, (u8*)"EXTENDED", WHITE, BLUE, 16, 0);

            LCD_ShowString(10, 28, (u8*)"1. Color Mode", (extended_menu_cursor == 0) ? CYAN : WHITE, BLACK, 16, 0);
            LCD_ShowString(10, 48, (u8*)"2. Music Mode", (extended_menu_cursor == 1) ? CYAN : WHITE, BLACK, 16, 0);
            LCD_ShowString(10, 68, (u8*)"3. Auto Bright", (extended_menu_cursor == 2) ? CYAN : WHITE, BLACK, 16, 0);
            LCD_ShowString(10, 88, (u8*)"4. Sound Detect", (extended_menu_cursor == 3) ? CYAN : WHITE, BLACK, 16, 0);

            LCD_DrawLine(0, 108, 127, 108, GRAY);
            LCD_ShowString(10, 112, (u8*)"K1:Sel  K2:Enter", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_COLOR:
            LCD_ShowString(24, 2, (u8*)"COLOR MODE", WHITE, BLUE, 16, 0);
            
            sprintf(buf, "Index: %d", color_index);
            LCD_ShowString(30, 50, (u8*)buf, WHITE, BLACK, 16, 0);
            
            // 显示颜色名称
            sprintf(buf, "Color: %s", color_modes[color_index].name);
            LCD_ShowString(20, 80, (u8*)buf, CYAN, BLACK, 16, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(10, 110, (u8*)"K1:Next  K2:Back", GRAY, BLACK, 12, 0);
            break;
            
        case MENU_MUSIC:
            LCD_ShowString(16, 2, (u8*)"MUSIC RHYTHM", WHITE, BLUE, 16, 0);
            // 校准和音乐律动的显示由Music_Rhythm_Update动态更新
            break;
            
        case MENU_AUTO_BRIGHTNESS:
            LCD_ShowString(10, 2, (u8*)"AUTO BRIGHTNESS", WHITE, BLUE, 16, 0);
            
            LCD_ShowString(20, 60, (u8*)"Sensing Light...", CYAN, BLACK, 16, 0);
            
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(25, 110, (u8*)"K1 / K2: Exit", GRAY, BLACK, 12, 0);
            break;

        case MENU_SOUND_DETECT:
            LCD_ShowString(10, 2, (u8*)"SOUND DETECT", WHITE, BLUE, 16, 0);
            // 实时显示由Sound_Detect_Update动态更新
            break;
        case MENU_VOICE:
            LCD_ShowString(12, 2, (u8*)"VOICE MODE", WHITE, BLUE, 16, 0);
            LCD_ShowString(6, 30, (u8*)"Say pinyin cmd:", WHITE, BLACK, 12, 0);
            LCD_ShowString(6, 46, (u8*)"RX:", CYAN, BLACK, 12, 0);
            LCD_ShowString(28, 46, (u8*)voice_rx_debug, WHITE, BLACK, 12, 0);
            LCD_ShowString(6, 68, (u8*)"Last:", CYAN, BLACK, 12, 0);
            LCD_ShowString(42, 68, (u8*)voice_last_cmd, WHITE, BLACK, 12, 0);
            LCD_DrawLine(0, 105, 127, 105, GRAY);
            LCD_ShowString(12, 110, (u8*)"K1/K2: Back", GRAY, BLACK, 12, 0);
            break;
    }
}

/**
 * @brief 语音模式更新（串口轮询接收 + LED状态）
 */
void Voice_Mode_Update(void) {
    uint32_t now = HAL_GetTick();
    uint8_t i;
    char rx_view[16];
    uint8_t tail_len;
    uint8_t start_idx;

    // 轮询接收LD3320发来的拼音命令文本（\r\n结尾）
    while (HAL_UART_Receive(&huart2, &voice_rx_byte, 1, 0) == HAL_OK) {
        if (voice_rx_byte == '\r') {
            continue;
        }
        if (voice_rx_byte == '\n') {
            if (voice_cmd_idx > 0) {
                voice_cmd_buf[voice_cmd_idx] = '\0';
                Voice_Process_Command(voice_cmd_buf);
                voice_cmd_idx = 0;
            }
            // 显示换行到达，便于确认帧结束
            if (voice_rx_debug_idx < (sizeof(voice_rx_debug) - 1U)) {
                voice_rx_debug[voice_rx_debug_idx++] = '|';
            }
            voice_rx_debug[voice_rx_debug_idx] = '\0';
            voice_rx_debug_updated = 1;
            continue;
        }

        if (voice_cmd_idx < (sizeof(voice_cmd_buf) - 1U)) {
            voice_cmd_buf[voice_cmd_idx++] = (char)voice_rx_byte;
        } else {
            voice_cmd_idx = 0;
        }

        // 原始串口数据显示（仅保留可打印ASCII）
        if (voice_rx_debug_idx >= (sizeof(voice_rx_debug) - 1U)) {
            for (i = 1; i < voice_rx_debug_idx; i++) {
                voice_rx_debug[i - 1] = voice_rx_debug[i];
            }
            voice_rx_debug_idx--;
        }
        if (voice_rx_byte >= 32U && voice_rx_byte <= 126U) {
            voice_rx_debug[voice_rx_debug_idx++] = (char)voice_rx_byte;
        } else {
            voice_rx_debug[voice_rx_debug_idx++] = '.';
        }
        voice_rx_debug[voice_rx_debug_idx] = '\0';
        voice_rx_debug_updated = 1;
    }

    if (voice_rx_debug_updated) {
        voice_rx_debug_updated = 0;
        tail_len = (uint8_t)strlen(voice_rx_debug);
        if (tail_len > 14U) {
            start_idx = (uint8_t)(tail_len - 14U);
            for (i = 0; i < 14U; i++) {
                rx_view[i] = voice_rx_debug[start_idx + i];
            }
            rx_view[14] = '\0';
        } else {
            for (i = 0; i < tail_len; i++) {
                rx_view[i] = voice_rx_debug[i];
            }
            for (; i < 14U; i++) {
                rx_view[i] = ' ';
            }
            rx_view[14] = '\0';
        }

        LCD_Fill(28, 46, 127, 58, BLACK);
        LCD_ShowString(28, 46, (u8*)rx_view, WHITE, BLACK, 12, 0);
    }

    // LED指示：命令执行后长亮2s，否则闪烁
    if (now < voice_led_hold_until) {
        Voice_LED_Set(1);
    } else if (now - voice_led_last_toggle >= 300U) {
        voice_led_last_toggle = now;
        voice_led_blink_state = !voice_led_blink_state;
        Voice_LED_Set(voice_led_blink_state);
    }
}

/**
 * @brief 执行语音命令
 */
void Voice_Process_Command(const char *cmd) {
    if (strcmp(cmd, "1") == 0) {
        WS2812_Set_All(LED_NUM, 255, 255, 255);
        strcpy(voice_last_cmd, "1:open");
    } else if (strcmp(cmd, "2") == 0) {
        WS2812_Clear(LED_NUM);
        strcpy(voice_last_cmd, "2:close");
    } else if (strcmp(cmd, "3") == 0) {
        WS2812_Set_All(LED_NUM, 255, 0, 0);
        strcpy(voice_last_cmd, "3:red");
    } else if (strcmp(cmd, "4") == 0) {
        WS2812_Set_All(LED_NUM, 0, 0, 255);
        strcpy(voice_last_cmd, "4:blue");
    } else if (strcmp(cmd, "5") == 0) {
        WS2812_Set_All(LED_NUM, 255, 255, 255);
        strcpy(voice_last_cmd, "5:white");
    } else {
        return;
    }

    // 命令执行时，语音LED长亮2秒
    voice_led_hold_until = HAL_GetTick() + 2000U;
    Voice_LED_Set(1);

    // 局部刷新命令显示区域，避免整屏闪烁
    LCD_Fill(42, 68, 127, 80, BLACK);
    LCD_ShowString(42, 68, (u8*)voice_last_cmd, WHITE, BLACK, 12, 0);
}

/**
 * @brief HSV转RGB颜色空间
 * @param h 色相 (0-1535)
 * @param s 饱和度 (0-255)
 * @param v 亮度 (0-255)
 * @param r 输出红色
 * @param g 输出绿色
 * @param b 输出蓝色
 */
void HSV_to_RGB(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region = h / 256;
    uint16_t remainder = (h % 256) * 6;

    uint8_t p = (v * (255 - s)) / 256;
    uint8_t q = (v * (255 - (s * remainder) / 1536)) / 256;
    uint8_t t = (v * (255 - (s * (1535 - remainder)) / 1536)) / 256;

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

/**
 * @brief 设置8x8矩阵中指定位置的LED颜色
 * @param row 行 (0-7)
 * @param col 列 (0-7)
 * @param r, g, b 颜色值
 */
void Matrix_Set_Pixel(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) {
    if (row >= 8 || col >= 8) return;
    // 同向横行布局：第0行是LED 0-7，第1行是LED 8-15，以此类推
    uint16_t index = row * 8 + col;
    WS2812_Set_Color(index, r, g, b);
}

/**
 * @brief 音乐律动逻辑更新 (8x8矩阵渐变版本)
 *
 * 算法说明：
 * 1. 进入音乐模式后，先进行5秒环境噪音采集
 *    - 在LCD上显示采集进度和计算过程
 *    - 采集完成后计算均值，乘以1.5作为噪音阈值
 * 2. 校准完成后，使用阈值判断是否触发音乐律动
 *    - ADC值低于阈值时不触发，LED保持微弱亮度
 *    - ADC值高于阈值时触发，根据强度变化颜色
 * 3. 8x8矩阵渐变效果：
 *    - 整个矩阵显示同一渐变颜色
 *    - 颜色根据音乐强度缓慢变化色相
 *    - 使用指数移动平均平滑颜色变化，避免闪烁
 */
void Music_Rhythm_Update(void) {
    static uint32_t last_sample_time = 0;
    static uint32_t last_lcd_update = 0;
    static uint8_t last_bar_length = 0;
    uint32_t now = HAL_GetTick();

    // 每100ms采样一次
    if (now - last_sample_time < 100) return;
    last_sample_time = now;

    uint32_t mic_val = Micphone_GetAverage(10);

    // 初始化校准（只执行一次）
    if (!music_cal_initialized) {
        Mic_Calibration_Init(&mic_cal);
        music_cal_initialized = 1;
        // 清空进度条区域
        LCD_Fill(15, 45, 112, 95, BLACK);
    }

    // 校准阶段
    if (mic_cal.state == MIC_CALIBRATING) {
        Mic_Calibration_Process(&mic_cal, mic_val);

        // 每500ms更新一次LCD显示
        if (now - last_lcd_update >= 500) {
            last_lcd_update = now;

            // 计算当前均值用于显示
            uint32_t current_avg = 0;
            if (mic_cal.count > 0) {
                current_avg = mic_cal.sum / mic_cal.count;
            }

            // 显示校准进度
            char buf[32];
            uint32_t elapsed = now - mic_cal.start_time;
            uint8_t progress = (elapsed * 100) / 5000;
            if (progress > 100) progress = 100;

            LCD_ShowString(10, 45, (u8*)"Calibrating...", WHITE, BLACK, 16, 0);

            sprintf(buf, "Time: %lu.%lus", elapsed / 1000, (elapsed % 1000) / 100);
            LCD_ShowString(10, 65, (u8*)buf, CYAN, BLACK, 16, 0);

            sprintf(buf, "ADC Avg: %lu", current_avg);
            LCD_ShowString(10, 85, (u8*)buf, YELLOW, BLACK, 16, 0);

            // 进度条
            uint8_t bar_width = (progress * 98) / 100;
            LCD_Fill(15, 105, 15 + bar_width, 110, GREEN);
        }

        // 校准完成
        if (mic_cal.state == MIC_CALIBRATED) {
            // 显示校准结果
            char buf[32];
            LCD_Clear(BLACK);

            // 重新绘制标题栏
            LCD_Fill(0, 0, 127, 20, BLUE);
            LCD_ShowString(16, 2, (u8*)"MUSIC RHYTHM", WHITE, BLUE, 16, 0);

            LCD_ShowString(10, 30, (u8*)"Calibration Done!", GREEN, BLACK, 16, 0);

            sprintf(buf, "Noise Avg: %lu", mic_cal.sum / mic_cal.count);
            LCD_ShowString(10, 50, (u8*)buf, WHITE, BLACK, 16, 0);

            sprintf(buf, "Threshold: %lu", mic_cal.noise_threshold);
            LCD_ShowString(10, 70, (u8*)buf, CYAN, BLACK, 16, 0);

            // 绘制进度条边框
            LCD_DrawLine(14, 95, 114, 95, GRAY);
            LCD_DrawLine(14, 105, 114, 105, GRAY);
            LCD_DrawLine(14, 95, 14, 105, GRAY);
            LCD_DrawLine(114, 95, 114, 105, GRAY);

            LCD_DrawLine(0, 115, 127, 115, GRAY);
            LCD_ShowString(25, 118, (u8*)"K1 / K2: Exit", GRAY, BLACK, 12, 0);

            HAL_Delay(1500); // 显示1.5秒

            // 清空进度条区域准备音乐律动显示
            LCD_Fill(15, 96, 113, 104, BLACK);
            last_bar_length = 0;
        }

        return;
    }

    // ========== 校准完成后的音乐律动逻辑 ==========

    // 计算与阈值的差值
    uint8_t intensity = 0;
    if (mic_val > mic_cal.noise_threshold) {
        uint32_t diff = mic_val - mic_cal.noise_threshold;
        // 映射到0-255，假设最大差值为2000
        intensity = (diff * 255) / 2000;
        if (intensity > 255) intensity = 255;
    }

    // 更新色相：有音乐时缓慢变化色相
    if (intensity > 10) {
        // 色相变化速度与强度成正比
        music_hue += intensity / 16;
        if (music_hue >= 1536) music_hue -= 1536;
    }

    // 平滑亮度：使用指数移动平均
    // 目标亮度：安静时30，最大音量时255
    uint8_t target_value = 30 + (intensity * 225) / 255;
    // 平滑系数：new = old * 0.7 + target * 0.3
    music_value = (music_value * 179 + target_value * 77) / 256;

    // 转换为RGB
    uint8_t r, g, b;
    HSV_to_RGB(music_hue, music_saturation, music_value, &r, &g, &b);

    // 设置整个8x8矩阵为同一颜色
    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            Matrix_Set_Pixel(row, col, r, g, b);
        }
    }
    WS2812_Show(LED_NUM);

    // 计算进度条长度 (最大长度 98)
    uint8_t bar_length = 0;
    if (intensity > 0) {
        bar_length = (intensity * 98) / 255;
        if (bar_length > 98) bar_length = 98;
    }

    // 更新LCD进度条显示 (15 到 113, y: 96 到 104)
    if (current_menu == MENU_MUSIC && bar_length != last_bar_length) {
        if (bar_length > last_bar_length) {
            LCD_Fill(15 + last_bar_length, 96, 15 + bar_length - 1, 104, CYAN);
        } else {
            LCD_Fill(15 + bar_length, 96, 15 + last_bar_length - 1, 104, BLACK);
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

/**
 * @brief 声音检测更新 - 实时显示ADC值
 *
 * 功能说明：
 * 1. 每200ms采样一次ADC值
 * 2. 在LCD上显示：
 *    - 当前ADC值 (0-4095)
 *    - 最大ADC值 (本次会话中的峰值)
 *    - 柱状图进度条 (可视化当前值)
 */
void Sound_Detect_Update(void) {
    static uint32_t last_sample_time = 0;
    static uint32_t max_value = 0;
    static uint32_t last_display_value = 0;
    char buf[32];
    uint32_t now = HAL_GetTick();

    // 每200ms采样一次
    if (now - last_sample_time < 200) return;
    last_sample_time = now;

    // 获取ADC值 (10次采样取平均)
    uint32_t mic_val = Micphone_GetAverage(10);

    // 更新最大值
    if (mic_val > max_value) {
        max_value = mic_val;
    }

    // 只在值变化时更新LCD，减少闪烁
    if (mic_val != last_display_value || max_value == mic_val) {
        last_display_value = mic_val;

        // 显示当前ADC值
        sprintf(buf, "ADC: %lu   ", mic_val);
        LCD_ShowString(10, 30, (u8*)buf, WHITE, BLACK, 16, 0);

        // 显示最大值
        sprintf(buf, "Max: %lu   ", max_value);
        LCD_ShowString(10, 50, (u8*)buf, YELLOW, BLACK, 16, 0);

        // 显示强度等级
        const char* level_str;
        uint16_t level_color;
        if (mic_val < 500) {
            level_str = "Level: Quiet  ";
            level_color = GREEN;
        } else if (mic_val < 1500) {
            level_str = "Level: Low    ";
            level_color = CYAN;
        } else if (mic_val < 2500) {
            level_str = "Level: Medium ";
            level_color = YELLOW;
        } else if (mic_val < 3500) {
            level_str = "Level: High   ";
            level_color = MAGENTA;
        } else {
            level_str = "Level: Very High";
            level_color = RED;
        }
        LCD_ShowString(10, 70, (u8*)level_str, level_color, BLACK, 16, 0);

        // 柱状图进度条 (0-4095 映射到 0-100像素)
        uint8_t bar_length = (mic_val * 100) / 4095;
        if (bar_length > 100) bar_length = 100;

        // 绘制进度条边框
        LCD_DrawLine(13, 90, 115, 90, GRAY);
        LCD_DrawLine(13, 100, 115, 100, GRAY);
        LCD_DrawLine(13, 90, 13, 100, GRAY);
        LCD_DrawLine(115, 90, 115, 100, GRAY);

        // 填充进度条
        if (bar_length > 0) {
            // 根据强度选择颜色
            uint16_t bar_color;
            if (mic_val < 1000) bar_color = GREEN;
            else if (mic_val < 2000) bar_color = CYAN;
            else if (mic_val < 3000) bar_color = YELLOW;
            else bar_color = RED;

            LCD_Fill(14, 91, 14 + bar_length, 99, bar_color);
        }
        // 清除多余部分
        if (bar_length < 100) {
            LCD_Fill(14 + bar_length, 91, 114, 99, BLACK);
        }
    }

    // 显示提示信息 (只绘制一次)
    static uint8_t info_drawn = 0;
    if (!info_drawn) {
        LCD_ShowString(10, 110, (u8*)"K1/K2: Back", GRAY, BLACK, 12, 0);
        info_drawn = 1;
    }
}

/**
 * @brief 重置声音检测状态 (退出时调用)
 */
void Sound_Detect_Reset(void) {
    // 静态变量会在下次进入时自动重置
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
