/*
 * UartController.h
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#ifndef UARTCONTROLLER_H_
#define UARTCONTROLLER_H_

#include "stm32g0xx_hal.h"

class UartController {
public:
	UartController(UART_HandleTypeDef* huart);
	virtual ~UartController();
private:
	UART_HandleTypeDef* huart_;
};

#endif /* UARTCONTROLLER_H_ */
