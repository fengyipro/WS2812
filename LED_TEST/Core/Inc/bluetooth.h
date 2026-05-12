#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "main.h"
#include "usart.h"

/**
 * @brief 蓝牙模块初始化（USART3）
 */
void Bluetooth_Init(void);

/**
 * @brief 轮询接收蓝牙数据并缓存数字命令
 */
void Bluetooth_Update(void);

/**
 * @brief 获取一个数字命令
 * @param cmd 输出命令值（0-9）
 * @return 1 有可读命令，0 无命令
 */
uint8_t Bluetooth_GetCommand(uint8_t *cmd);

#endif /* __BLUETOOTH_H */
