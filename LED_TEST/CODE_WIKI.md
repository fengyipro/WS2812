# LED_TEST Code Wiki

## 1. 项目概览

本仓库是一个基于 STM32CubeMX 生成工程的 STM32F103C8T6（Cortex‑M3）固件项目，主要功能包括：

- WS2812 灯带（8x8=64 颗）TIM1 PWM + DMA 驱动
- 1.44 寸（128x128）TFT LCD（模拟 SPI）UI 菜单
- 双按键（PB6/PB7）外部中断 + 软件消抖 + 蜂鸣器反馈
- 麦克风模块（PA6 ADC1_IN6）采样与环境噪声校准，用于“音乐律动/声音检测”
- 光敏模块（PB0 数字输入）用于“自动调光”
- 语音控制（USART2 9600）轮询接收文本指令（预留 LD3320 或其它语音模块）

代码主入口与状态机在 [main.c](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c)。

## 2. 快速开始

### 2.1 工程与工具链

- 工程类型：STM32CubeMX + Keil uVision（MDK-ARM）
- 工程文件：
  - CubeMX： [LED_TEST.ioc](file:///d:/文件集合/电赛校内赛项目/LED_TEST/LED_TEST.ioc)
  - Keil： [LED_TEST.uvprojx](file:///d:/文件集合/电赛校内赛项目/LED_TEST/MDK-ARM/LED_TEST.uvprojx)
- 关键外设配置可在 IOC 中确认（ADC1、TIM1+DMA、USART2/3、EXTI、GPIO）。

### 2.2 构建与烧录（Keil）

- 使用 Keil 打开 `MDK-ARM/LED_TEST.uvprojx`
- 选择 Target `LED_TEST`
- Build 后会在 `MDK-ARM/LED_TEST/` 生成 `LED_TEST.hex` 等产物
- 使用 ST-LINK / J-LINK 进行下载调试

### 2.3 重新生成代码（CubeMX）

- 使用 CubeMX 打开 `LED_TEST.ioc`
- 保持 “Keep User Code” 以避免覆盖 `/* USER CODE BEGIN */` 区域
- 生成代码后，检查以下点是否保持一致：
  - TIM1 + DMA 配置（模式/通道/中断）
  - GPIO 引脚复用（PB13 TIM1_CH1N）
  - USART2/3 波特率与引脚

## 3. 硬件与引脚映射

详细说明见 [readme.md](file:///d:/文件集合/电赛校内赛项目/LED_TEST/readme.md)。

- LCD（模拟 SPI）
  - RST：PA0
  - DC：PA1
  - CS：PB8
  - BLK：PB9
  - SCLK：PA5
  - MOSI：PA7
- WS2812
  - 数据：PB13（TIM1_CH1N）
- 声音传感器（模拟）
  - PA6（ADC1_IN6）
- 光敏模块（数字）
  - PB0（上拉输入）
- 按键
  - KEY1：PB6（EXTI，下降沿）
  - KEY2：PB7（EXTI，下降沿）
- 蜂鸣器
  - PB12（低电平有效）
- 语音模块
  - USART2：PA2/PA3（9600）
  - USART3：PB10/PB11（9600，当前代码未使用）
- 语音状态指示 LED
  - PA4（低电平点亮，高电平熄灭）

## 4. 仓库目录结构

- Core/
  - Inc/：应用层头文件与自定义驱动头文件（`lcd.h/ws2812.h/...`）
  - Src/：应用层源码与自定义驱动实现（`main.c/lcd.c/ws2812.c/...`）
- Drivers/：ST 官方 HAL + CMSIS（体量大，通常不手改）
- MDK-ARM/：Keil 工程目录（构建产物位于 `MDK-ARM/LED_TEST/`）
- LED_TEST.ioc：CubeMX 配置文件

## 5. 运行时架构（控制流与数据流）

### 5.1 顶层状态机

菜单/模式以枚举 `MenuState_t` 管理：主菜单、亮度、扩展菜单、颜色、音乐律动、自动调光、声音检测、语音模式等。

- 定义与全局状态： [main.c:L40-L111](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L40-L111)
- 主循环： [main.c:L183-L204](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L183-L204)

主循环每 10ms 运行一次，做两件事：

- `Handle_Keys()`：消费按键事件并进行 UI/模式切换
- 根据 `current_menu` 执行对应模式的周期更新：
  - `Music_Rhythm_Update()`：100ms 采样 + 动态 LED/LCD
  - `Auto_Brightness_Update()`：500ms 采样光敏
  - `Sound_Detect_Update()`：200ms 采样麦克风并绘制柱状图
  - `Voice_Mode_Update()`：轮询串口接收指令并更新 LED 状态

### 5.2 按键输入（中断 -> 事件标志 -> 主循环处理）

- EXTI 中断回调只做“置位事件标志 + 消抖”： [main.c:L254-L270](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L254-L270)
- 主循环中 `Handle_Keys()` 根据状态机执行动作： [main.c:L272-L408](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L272-L408)

这种模式避免在中断中执行复杂逻辑（较安全），但当前仍存在一个“阻塞点”（见风险章节）。

### 5.3 WS2812（TIM1 PWM + DMA）

- 参数与上限： [ws2812.h](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Inc/ws2812.h)
  - `LED_MAX_COUNT=64`，与 `main.c` 中 8x8 矩阵逻辑一致
  - `WS2812_T0H/WS2812_T1H` 依赖 TIM1 周期配置（`Period=90-1`）
- DMA 发送与回调： [ws2812.c:L4-L87](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/ws2812.c#L4-L87)
- TIM1 配置与 DMA 通道： [tim.c:L27-L147](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/tim.c#L27-L147)
- DMA 中断入口： [stm32f1xx_it.c:L201-L213](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/stm32f1xx_it.c#L201-L213)

数据流简图：

1. 应用层调用 `WS2812_Set_Color/WS2812_Set_All` 写入 `ws2812_pixel_data[]`
2. 调用 `WS2812_Show()` 启动 `HAL_TIMEx_PWMN_Start_DMA()` 输出 PWM 占空比序列
3. DMA 完成后，在回调里 Stop DMA 并清 `busy` 标志

### 5.4 LCD（模拟 SPI）与 UI

- LCD 驱动在 [lcd.c](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/lcd.c) / [lcd.h](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Inc/lcd.h)
- UI 刷新在 [UI_Refresh](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L410-L505)

UI 采用“整屏清空 + 重新绘制”为主；在语音/音乐等模式下，对部分区域做 `LCD_Fill` 局部更新以减少闪烁。

### 5.5 麦克风（ADC 轮询）与校准

- ADC 配置： [adc.c](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/adc.c)
- 采样与均值： [micphone.c:L11-L31](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/micphone.c#L11-L31)
- 5 秒环境噪声校准： [micphone.c:L33-L79](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/micphone.c#L33-L79)
- 音乐律动使用校准阈值： [main.c:L664-L821](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L664-L821)

### 5.6 光敏（数字输入）与自动调光

- 输入配置： [gpio.c:L65-L69](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/gpio.c#L65-L69)
- 读输入并判断： [light.c:L11-L22](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/light.c#L11-L22)
- 自动调光逻辑： [main.c:L823-L838](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L823-L838)

### 5.7 蜂鸣器（GPIO）

- 宏定义与“低电平有效”约定： [buzzer.h](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Inc/buzzer.h)
- 短鸣实现： [buzzer.c:L11-L18](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/buzzer.c#L11-L18)

### 5.8 语音模式（USART2 轮询 + 简单指令协议）

- USART2 初始化： [usart.c:L30-L58](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/usart.c#L30-L58)
- 轮询接收并以 `\r\n` 作为帧结束： [main.c:L507-L590](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L507-L590)
- 解析指令（当前为 `"1"..."5"`）： [main.c:L592-L622](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L592-L622)

可视为一个极简文本协议：

- 一条指令是一行 ASCII 文本，以 `\n` 结束
- 支持命令：1 开灯白 / 2 关灯 / 3 红 / 4 蓝 / 5 白

## 6. 关键配置与常量

### 6.1 WS2812 时序

- TIM1 周期：`Period=90-1`（72MHz 下约 1.25us）
- PWM 占空比计数：
  - `WS2812_T0H=25`、`WS2812_T1H=58`（在 [ws2812.h](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Inc/ws2812.h)）

如果更换主频、Prescaler 或 Period，需要同时重新标定 `T0H/T1H`。

### 6.2 采样/刷新频率

- 主循环：10ms（[main.c:L185-L200](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L185-L200)）
- 音乐律动 ADC：100ms
- 声音检测 ADC：200ms
- 自动调光：500ms

## 7. 常见问题与排错

- WS2812 全灭/乱闪
  - 检查 PB13 是否为 TIM1_CH1N、是否正确接地与供电
  - 检查 `WS2812_T0H/T1H` 与 `TIM1 Period` 是否匹配
  - 检查 DMA1_Channel2 中断是否启用（[dma.c:L39-L49](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/dma.c#L39-L49)）
- LCD 无显示
  - 检查 BLK 是否拉高（背光）
  - 检查 CS/DC/RST 引脚与连线
  - LCD 驱动是模拟 SPI，SCLK/MOSI 需为推挽输出（[gpio.c](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/gpio.c)）
- 按键无响应
  - 确认 PB6/PB7 上拉与下降沿中断（[gpio.c:L78-L87](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/gpio.c#L78-L87)）
  - 确认 `EXTI9_5_IRQHandler` 中两个 pin 都调用了 `HAL_GPIO_EXTI_IRQHandler`（[stm32f1xx_it.c:L215-L228](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/stm32f1xx_it.c#L215-L228)）

## 8. 项目评价、不足与潜在风险

### 8.1 工程质量与优点

- 模块拆分清晰：LCD/WS2812/麦克风/光敏/蜂鸣器都独立成对 `*.c/*.h`
- 外设用法合理：WS2812 采用 TIM + DMA，CPU 占用低；按键用 EXTI，提高响应性
- UI 交互完整：菜单、局部刷新、状态切换逻辑明确

### 8.2 不足与风险清单（按优先级）

- 阻塞式延时出现在“事件处理路径”（实时性风险）
  - `Buzzer_Beep_Short()` 使用 `HAL_Delay(50)`（[buzzer.c:L14-L17](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/buzzer.c#L14-L17)），而它在 `Handle_Keys()` 中被调用（[main.c:L275-L340](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L275-L340)），会将主循环直接阻塞 50ms，影响音乐律动/语音接收/LED 刷新节奏。
- 生成配置存在不一致迹象（维护风险）
  - IOC 中 DMA 设为 `DMA_CIRCULAR`（[LED_TEST.ioc:L11-L18](file:///d:/文件集合/电赛校内赛项目/LED_TEST/LED_TEST.ioc#L11-L18)），但 `tim.c` 里 DMA 模式是 `DMA_NORMAL`（[tim.c:L104-L112](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/tim.c#L104-L112)）。这种差异会导致后续重新生成代码时出现行为变化或覆盖问题。
- 语音输入协议健壮性一般（鲁棒性风险）
  - 语音命令缓冲区满后直接清零索引（[main.c:L537-L541](file:///d:/文件集合/电赛校内赛项目/LED_TEST/Core/Src/main.c#L537-L541)），缺少丢帧标记/错误提示；若噪声串口输入较多，可能导致 UI 显示与实际命令不同步。

### 8.3 建议的改进方向（可落地）

- 将蜂鸣器短鸣改为非阻塞（TIM/软定时器），避免主循环卡顿
- 增加“配置一致性检查”清单：重新生成 CubeMX 后需手工确认 DMA 模式、TIM 通道与时序宏
- 将模式更新周期/阈值参数集中到单一配置头文件，便于调参
