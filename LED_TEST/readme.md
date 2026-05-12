# 简单说明

## 0. 快速入口

- 使用操作说明：见 [操作指南.md](操作指南.md)

## 1. 项目概述

本项目基于 STM32F103C8T6，实现：

- LCD 菜单 UI + 状态机交互
- WS2812 灯带驱动（TIM1 PWM + DMA）
- 按键（EXTI）与蜂鸣器反馈（非阻塞短鸣）
- 蓝牙串口数字控制（USART3，0=退出，1..n=当前页面功能号）
- 光敏电阻 ADC 线性自动调光 + 进度条 UI
- 麦克风采样 + 音乐律动（自适应噪声阈值，输出 12 灯整环同色）
- 语音串口页（USART2，显示最近命令，并可被蓝牙数字触发 1..5）

## 2. 硬件连接说明

- LCD（1.44 寸 128x128）：
  - RST: PA0
  - DC: PA1
  - CS: PB8
  - BLK: PB9
  - SCLK: PA5（软件 SPI）
  - MOSI: PA7（软件 SPI）
- WS2812：
  - 数据引脚：PB13（TIM1_CH1N）
- 麦克风（模拟）：
  - PA6（ADC1_IN6）
- 光敏（模拟）：
  - PB0（ADC1_IN8）
- 按键：
  - KEY1：PB6（EXTI，下降沿）
  - KEY2：PB7（EXTI，下降沿）
- 蜂鸣器：
  - PB12（低电平有效）
- 蓝牙串口模块（透明串口）：
  - USART3_TX：PB10
  - USART3_RX：PB11

相关代码：

- 引脚宏定义见 [main.h](Core/Inc/main.h#L59-L80)
- USART2/USART3 初始化与引脚见 [usart.c](Core/Src/usart.c#L27-L146)

## 3. 运行与烧录

- Keil 工程：`MDK-ARM/LED_TEST.uvprojx`
- CubeMX 工程：`LED_TEST.ioc`
- 串口参数（USART2/USART3）：9600, 8N1

## 4. 菜单与控制逻辑（概要）

菜单结构（开机默认主菜单）：

- MAIN MENU
  - 1. Brightness
  - 2. Voice Ctrl
  - 3. Extended
- EXTENDED
  - 1. Color Mode
  - 2. Music Mode
  - 3. Auto Bright
  - 4. Exit

控制规则：

- 按键：KEY1 通常用于选择/下一项；KEY2 通常用于进入/返回
- 蓝牙：`0` 在任意页面为退出；`1..n` 在当前页面表示第 n 个功能号（只执行“最新一次”命令）

相关代码：

- 菜单状态枚举与 UI 绘制见 [main.c:UI_Refresh](Core/Src/main.c#L572-L671)
- 按键状态机见 [main.c:Handle_Keys](Core/Src/main.c#L256-L418)
- 蓝牙命令映射见 [main.c:Handle_Bluetooth_Commands](Core/Src/main.c#L420-L570)

## 5. 模块实现说明（含相关代码与注释片段）

### 5.1 UI / 状态机（main.c）

实现要点：

- 使用 `MenuState_t current_menu` 作为状态机核心；KEY 与蓝牙数字都只是在不同状态下触发不同跳转
- UI 刷新集中在 `UI_Refresh()`，在页面切换或关键信息变化时调用

相关代码：

- 菜单枚举与关键变量见 [main.c](Core/Src/main.c#L41-L109)
- UI 刷新见 [main.c:UI_Refresh](Core/Src/main.c#L572-L671)

注释片段（工程内原注释）：

```c
/**
 * @brief 刷新LCD界面显示 (适配1.44寸 128x128 屏幕)
 */
void UI_Refresh(void) {
```

### 5.2 按键（EXTI + 主循环处理）

实现要点：

- EXTI 中断仅做“置位标志”，避免在中断里做耗时逻辑
- 200ms 软件消抖，降低误触发

相关代码：

- 中断回调与消抖： [main.c:HAL_GPIO_EXTI_Callback](Core/Src/main.c#L256-L273)
- 按键处理与菜单跳转： [main.c:Handle_Keys](Core/Src/main.c#L275-L418)

注释片段：

```c
/**
 * @brief 外部中断回调函数，处理按键触发
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
```

### 5.3 蓝牙数字控制（USART3 中断接收 + 命令队列）

实现要点：

- USART3 字节中断接收；仅识别 `'0'..'9'`
- 队列满时丢最旧，保证新点击尽可能生效
- 处理层采用“只执行最新命令”策略，减少连发造成的抖动与刷屏

相关代码：

- USART3 中断处理： [stm32f1xx_it.c:USART3_IRQHandler](Core/Src/stm32f1xx_it.c#L234-L237)
- 蓝牙接收/队列： [bluetooth.c](Core/Src/bluetooth.c#L1-L81)
- 命令与页面行为映射： [main.c:Handle_Bluetooth_Commands](Core/Src/main.c#L420-L570)

注释片段：

```c
/**
 * @brief 处理蓝牙数字命令（0=退出，1..n=当前界面功能号）
 */
void Handle_Bluetooth_Commands(void) {
```

### 5.4 蜂鸣器（非阻塞短鸣）

实现要点：

- `Buzzer_Beep_Short()` 只设置“截止 tick”，不阻塞主循环
- `Buzzer_Update()` 放在 SysTick 中调用，避免 UI 刷新导致蜂鸣器变“长鸣”

相关代码：

- 蜂鸣器实现： [buzzer.c](Core/Src/buzzer.c#L1-L37)
- SysTick 调用： [stm32f1xx_it.c:SysTick_Handler](Core/Src/stm32f1xx_it.c#L183-L196)

注释片段：

```c
/**
 * @brief 蜂鸣器短鸣反馈（非阻塞）
 */
void Buzzer_Beep_Short(void) {
```

### 5.5 WS2812 驱动（TIM1 PWM + DMA）

实现要点：

- 将 GRB 位流编码成 PWM compare 值数组，通过 DMA 发送
- 发送完成回调里停止 PWM，避免持续输出干扰

相关代码：

- 驱动实现： [ws2812.c](Core/Src/ws2812.c#L1-L87)
- 接口定义： [ws2812.h](Core/Inc/ws2812.h#L7-L45)

注释片段：

```c
// PWM数据缓存：每个LED占24位 (G, R, B) + 复位信号
static uint16_t ws2812_pixel_data[LED_MAX_COUNT * 24 + WS2812_RESET_LEN];
```

### 5.6 光敏 ADC 与自动调光（Auto Bright）

实现要点：

- 光敏走 ADC1_IN8（PB0），提供 `Light_GetAverage(samples)` 读取平均值
- Auto Bright 采用自适应学习 `light_min/light_max`，并做平滑 `light_smooth`，兼容不同模块方向/量程
- LED 亮度与进度条 UI 都来自同一套映射

相关代码：

- 光敏采样： [light.c](Core/Src/light.c#L1-L32)
- 自动调光算法与 UI： [main.c:Auto_Brightness_Update](Core/Src/main.c#L952-L1019)

注释片段：

```c
/**
 * @brief 自动亮度更新
 */
void Auto_Brightness_Update(void) {
```

### 5.7 麦克风采样与音乐律动（Music Mode）

实现要点：

- 麦克风走 ADC1_IN6（PA6），提供 `Micphone_GetAverage(samples)`
- 进入音乐模式后先进行 5 秒噪声校准（得到初始阈值）
- 校准完成后：在线估计噪声 `noise` 与阈值 `threshold`，并用 `peak` 自适应归一化强度，提升不同音乐/环境下的可用性
- 输出为 12 灯圆环整环同色：`WS2812_Set_All(12, r, g, b)`

相关代码：

- 麦克风采样与校准： [micphone.c](Core/Src/micphone.c#L1-L84)
- 音乐律动： [main.c:Music_Rhythm_Update](Core/Src/main.c#L785-L950)

注释片段：

```c
/**
 * @brief 音乐律动逻辑更新 (8x8矩阵渐变版本)
 *
 * 算法说明：
 * 1. 进入音乐模式后，先进行5秒环境噪音采集
 * ...
 */
void Music_Rhythm_Update(void) {
```

### 5.8 Voice 模式（Voice Ctrl）

实现要点：

- USART2 轮询接收（0 超时），以 `\r\n` 作为一条命令的结束
- 为避免屏幕刷屏，仅缓存有效数字字符（'1'..'5'）
- 执行命令后局部刷新“Last cmd”区域

相关代码：

- 接收与 LED 指示： [main.c:Voice_Mode_Update](Core/Src/main.c#L673-L711)
- 命令映射与局部刷新： [main.c:Voice_Process_Command](Core/Src/main.c#L713-L743)

注释片段：

```c
// 仅缓存有效数字字符，避免串口噪声导致屏幕刷屏
if (voice_rx_byte >= '1' && voice_rx_byte <= '5') {
```
