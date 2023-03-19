/*
 * UartController.cpp
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#include "UartController.h"

UartController::UartController(UART_HandleTypeDef* huart) {
	this->huart_ = huart;

}

UartController::~UartController() {
	// TODO Auto-generated destructor stub
}

