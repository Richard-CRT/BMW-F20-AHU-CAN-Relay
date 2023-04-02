/*
 * CanController.cpp
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#include <can_controller_t.h>

can_controller_t::can_controller_t()
{
}

can_controller_t::~can_controller_t()
{
	// TODO Auto-generated destructor stub
}

void can_controller_t::init(FDCAN_HandleTypeDef* hfdcan)
{
	this->hfdcan = hfdcan;
}
