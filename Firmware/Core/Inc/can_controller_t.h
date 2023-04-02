/*
 * CanController.h
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#ifndef CAN_CONTROLLER_T_H_
#define CAN_CONTROLLER_T_H_

#include "stm32g0xx_hal.h"

class can_controller_t {
public:
	can_controller_t();
	virtual ~can_controller_t();

	void init(FDCAN_HandleTypeDef* hfdcan);
private:
	FDCAN_HandleTypeDef* hfdcan;
};

#endif /* CAN_CONTROLLER_T_H_ */
