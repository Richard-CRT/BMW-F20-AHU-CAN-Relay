/*
 * CanController.cpp
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#include "CanController.h"

CanController::CanController(FDCAN_HandleTypeDef* hfdcan)
{
	this->hfdcan_ = hfdcan;
}

CanController::~CanController()
{
	// TODO Auto-generated destructor stub
}
