/*
 * uart_controllers.h
 *
 *  Created on: Apr 1, 2023
 *      Author: richa
 */

#ifndef INC_UART_CONTROLLERS_H_
#define INC_UART_CONTROLLERS_H_

#include "uart_controller_t.h"

/**
 * \param huart UART Handle that was used
 * \brief UART Transmit Complete Interrupt Callback
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);

/**
 * \param huart UART Handle that was used
 * \brief UART Receive Complete Interrupt Callback
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

/**
 * \param huart UART Handle that was used
 * \brief UART Error Interrupt Callback
 *
 * Calls main error handler to prompt debugging
 * \see Error_Handler()
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

#endif /* INC_UART_CONTROLLERS_H_ */
