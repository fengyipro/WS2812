说明:
PA2：UART2_TX，复用推挽输出，语音模块串口发送
PA3：UART2_RX，上拉输入，语音模块串口接收
PA4：GPIO_Output，推挽输出，语音状态 LED
PA6：ADC_IN，模拟输入，声音传感器采集
PB0：GPIO_Input，上拉输入，光敏传感器数字信号
PB7：GPIO_Input/EXTI，上拉输入，按键 2（支持中断）
PB6：GPIO_Input/EXTI，上拉输入，按键 1（支持中断）
PB10：UART3_TX，复用推挽输出，蓝牙串口发送
PB11：UART3_RX，上拉输入，蓝牙串口接收
PB12：GPIO_Output，推挽输出，蜂鸣器控制
PB13：TIM1_CH1N，PWM输出，WS2812 RGB 灯带驱动
PA0/PA1/PA5/PA7/PB8~PB9：GPIO_Output，推挽输出，TFT 屏并行控制，其中PA5PA7配置SPI
电源状态led:通电源即亮，无引脚配置  