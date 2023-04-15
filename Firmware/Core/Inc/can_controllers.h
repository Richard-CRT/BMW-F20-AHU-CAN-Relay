/*
 * can_controllers.h
 *
 *  Created on: Apr 1, 2023
 *      Author: richa
 */

#ifndef INC_CAN_CONTROLLERS_H_
#define INC_CAN_CONTROLLERS_H_

#include "can_controller_t.h"

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);

/**
 * \param hfdcan FDCAN Handle that was used
 * \brief FDCAN Error Interrupt Callback
 *
 */
void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan);

#endif /* INC_CAN_CONTROLLERS_H_ */
