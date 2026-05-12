#include "bluetooth.h"

#define BT_CMD_QUEUE_SIZE 32

static uint8_t bt_rx_byte = 0;
static uint8_t bt_cmd_queue[BT_CMD_QUEUE_SIZE];
static uint8_t bt_head = 0;
static uint8_t bt_tail = 0;

static HAL_StatusTypeDef BT_StartRx(void) {
    return HAL_UART_Receive_IT(&huart3, &bt_rx_byte, 1);
}

static uint8_t BT_Queue_IsFull(void) {
    return (uint8_t)((bt_head + 1U) % BT_CMD_QUEUE_SIZE) == bt_tail;
}

static uint8_t BT_Queue_IsEmpty(void) {
    return bt_head == bt_tail;
}

static void BT_Queue_Push(uint8_t value) {
    if (BT_Queue_IsFull()) {
        bt_tail = (uint8_t)((bt_tail + 1U) % BT_CMD_QUEUE_SIZE);
    }

    bt_cmd_queue[bt_head] = value;
    bt_head = (uint8_t)((bt_head + 1U) % BT_CMD_QUEUE_SIZE);
}

static uint8_t BT_Queue_Pop(uint8_t *value) {
    if (BT_Queue_IsEmpty() || value == 0) {
        return 0;
    }

    *value = bt_cmd_queue[bt_tail];
    bt_tail = (uint8_t)((bt_tail + 1U) % BT_CMD_QUEUE_SIZE);
    return 1;
}

void Bluetooth_Init(void) {
    bt_head = 0;
    bt_tail = 0;
    bt_rx_byte = 0;

    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    __HAL_UART_CLEAR_OREFLAG(&huart3);
    BT_StartRx();
}

void Bluetooth_Update(void) {
    if (huart3.RxState != HAL_UART_STATE_BUSY_RX) {
        __HAL_UART_CLEAR_OREFLAG(&huart3);
        BT_StartRx();
    }
}

uint8_t Bluetooth_GetCommand(uint8_t *cmd) {
    return BT_Queue_Pop(cmd);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart != &huart3) {
        return;
    }

    if (bt_rx_byte >= '0' && bt_rx_byte <= '9') {
        BT_Queue_Push((uint8_t)(bt_rx_byte - '0'));
    }
    BT_StartRx();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart != &huart3) {
        return;
    }

    __HAL_UART_CLEAR_OREFLAG(&huart3);
    BT_StartRx();
}
