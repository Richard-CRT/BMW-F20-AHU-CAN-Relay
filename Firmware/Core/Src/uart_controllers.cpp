/*
 * uart_controllers.cpp
 *
 *  Created on: Apr 2, 2023
 *      Author: richa
 */

#include "uart_controllers.h"

uart_controller_t uart_controller_1;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
		uart_controller_1.tx_cplt_callback();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
		uart_controller_1.rx_cplt_callback();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	// Need to know what's causing this so call handler
	Error_Handler();
}
