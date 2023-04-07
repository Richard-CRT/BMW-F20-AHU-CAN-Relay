/*
 * rx_can_message_t.h
 *
 *  Created on: Apr 8, 2023
 *      Author: richa
 */

#ifndef INC_RX_CAN_MESSAGE_T_H_
#define INC_RX_CAN_MESSAGE_T_H_

#include "stm32g0xx_hal.h"

typedef struct {
	FDCAN_RxHeaderTypeDef RxHeader;
	uint8_t RxData[8];
} rx_can_message_t;


#endif /* INC_RX_CAN_MESSAGE_T_H_ */
