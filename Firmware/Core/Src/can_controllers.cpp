/*
 * can_controllers.cpp
 *
 *  Created on: Apr 2, 2023
 *      Author: richa
 */

#include "can_controllers.h"

can_controller_t can_controller_1;
can_controller_t can_controller_2;

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	if (hfdcan->Instance == FDCAN1)
		can_controller_1.rx_fifo_0_callback(RxFifo0ITs);
	else if (hfdcan->Instance == FDCAN2)
		can_controller_2.rx_fifo_0_callback(RxFifo0ITs);
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
	if (hfdcan->Instance == FDCAN1)
		can_controller_1.rx_fifo_1_callback(RxFifo1ITs);
	else if (hfdcan->Instance == FDCAN2)
		can_controller_2.rx_fifo_1_callback(RxFifo1ITs);
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
{
	if (hfdcan->Instance == FDCAN1)
		can_controller_1.error_callback();
	else if (hfdcan->Instance == FDCAN2)
		can_controller_2.error_callback();
}
