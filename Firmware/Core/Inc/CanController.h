/*
 * CanController.h
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#ifndef CANCONTROLLER_H_
#define CANCONTROLLER_H_

#include "stm32g0xx_hal.h"

class CanController {
public:
	CanController(FDCAN_HandleTypeDef* hfdcan);
	virtual ~CanController();
private:
	FDCAN_HandleTypeDef* hfdcan_;
};

#endif /* CANCONTROLLER_H_ */
