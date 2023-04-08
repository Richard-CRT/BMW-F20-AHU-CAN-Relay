/*
 * CanController.cpp
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#include <can_controller_t.h>

#include "uart_controllers.h"
#include "main.h"

extern uart_controller_t uart_controller_1;

can_controller_t::can_controller_t() :
		hfdcan(NULL), rx_buf_0(fifo_buf_t<rx_can_message_t, 256>(false)), // Configured to discard new data when full
		rx_buf_1(fifo_buf_t<rx_can_message_t, 256>(false)) // Configured to discard new data when full
{
}

can_controller_t::~can_controller_t()
{
	// TODO Auto-generated destructor stub
}

void can_controller_t::init(FDCAN_HandleTypeDef *hfdcan)
{
	this->hfdcan = hfdcan;

	/*

	 // ##-1 Configure the FDCAN filters ########################################
	 // Configure Rx filter
	 this->sFilterConfig.IdType = FDCAN_STANDARD_ID;
	 this->sFilterConfig.FilterIndex = 0;
	 this->sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	 this->sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	 this->sFilterConfig.FilterID1 = 0x321;
	 this->sFilterConfig.FilterID2 = 0x7FF;
	 if (HAL_FDCAN_ConfigFilter(hfdcan, &this->sFilterConfig) != HAL_OK)
	 {
	 Error_Handler();
	 }

	 // Configure extended ID reception filter to Rx FIFO 1
	 this->sFilterConfig.IdType = FDCAN_EXTENDED_ID;
	 this->sFilterConfig.FilterIndex = 0;
	 this->sFilterConfig.FilterType = FDCAN_FILTER_RANGE_NO_EIDM;
	 this->sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
	 this->sFilterConfig.FilterID1 = 0x1111111;
	 this->sFilterConfig.FilterID2 = 0x2222222;
	 if (HAL_FDCAN_ConfigFilter(hfdcan, &this->sFilterConfig) != HAL_OK)
	 {
	 Error_Handler();
	 }

	 */

	/* Configure global filter:
	 Filter all remote frames with STD and EXT ID
	 Reject non matching frames with STD ID and EXT ID */
	if (HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_ACCEPT_IN_RX_FIFO0,
	FDCAN_ACCEPT_IN_RX_FIFO0,
	FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
		Error_Handler();

	/*##-2 Start FDCAN controller (continuous listening CAN bus) ##############*/
	if (HAL_FDCAN_Start(hfdcan) != HAL_OK)
		Error_Handler();

	if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0)
			!= HAL_OK)
		Error_Handler();

	if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0)
			!= HAL_OK)
		Error_Handler();
}

void can_controller_t::send_message(FDCAN_TxHeaderTypeDef* txHeader, uint8_t* txData)
{
	if (HAL_FDCAN_AddMessageToTxFifoQ(this->hfdcan, txHeader, txData)
			!= HAL_OK)
		Error_Handler();
}

void can_controller_t::send_copy_of_rx_message(rx_can_message_t* rx_can_message)
{
	FDCAN_TxHeaderTypeDef TxHeader;
	TxHeader.Identifier = rx_can_message->RxHeader.Identifier;
	TxHeader.IdType = rx_can_message->RxHeader.IdType;
	TxHeader.TxFrameType = rx_can_message->RxHeader.RxFrameType;
	TxHeader.DataLength = rx_can_message->RxHeader.DataLength;
	TxHeader.ErrorStateIndicator = rx_can_message->RxHeader.ErrorStateIndicator;
	TxHeader.BitRateSwitch = rx_can_message->RxHeader.BitRateSwitch;
	TxHeader.FDFormat = rx_can_message->RxHeader.FDFormat;
	TxHeader.TxEventFifoControl = rx_can_message->RxHeader.Identifier;
	TxHeader.MessageMarker = 0;

	if (HAL_FDCAN_AddMessageToTxFifoQ(this->hfdcan, &TxHeader, rx_can_message->RxData)
			!= HAL_OK)
		Error_Handler();
}

bool can_controller_t::read_message_0(rx_can_message_t *rx_can_message)
{
	// No need to disable interrupt as this is the only code that can remove
	// from the circ buffer
	bool success = this->rx_buf_0.pop(rx_can_message);
	return success;
}

bool can_controller_t::read_message_1(rx_can_message_t *rx_can_message)
{
	// No need to disable interrupt as this is the only code that can remove
	// from the circ buffer
	bool success = this->rx_buf_1.pop(rx_can_message);
	return success;
}

void can_controller_t::rx_fifo_0_callback(uint32_t RxFifo0ITs)
{
	if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
	{
		rx_can_message_t rx_can_message;

		/* Retrieve Rx messages from RX FIFO0 */
		if (HAL_FDCAN_GetRxMessage(this->hfdcan, FDCAN_RX_FIFO0,
				&rx_can_message.RxHeader, rx_can_message.RxData) != HAL_OK)
			Error_Handler();

		this->rx_buf_0.push(rx_can_message);
	}
}

void can_controller_t::rx_fifo_1_callback(uint32_t RxFifo1ITs)
{
	if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET)
	{
		rx_can_message_t rx_can_message;

		/* Retrieve Rx messages from RX FIFO0 */
		if (HAL_FDCAN_GetRxMessage(this->hfdcan, FDCAN_RX_FIFO1,
				&rx_can_message.RxHeader, rx_can_message.RxData) != HAL_OK)
			Error_Handler();

		this->rx_buf_1.push(rx_can_message);
	}
}

// ##################################################################
//    Error
// ##################################################################

void can_controller_t::error_callback()
{
}
