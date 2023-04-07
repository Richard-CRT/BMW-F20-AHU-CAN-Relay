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
		hfdcan(NULL),
		rx_buf_0(fifo_buf_t<rx_can_message_t>(true)), // Configured to keep new data
		rx_buf_1(fifo_buf_t<rx_can_message_t>(true)) // Configured to keep new data
{
}

can_controller_t::~can_controller_t()
{
	// TODO Auto-generated destructor stub
}

void can_controller_t::init(FDCAN_HandleTypeDef *hfdcan)
{
	this->hfdcan = hfdcan;

	// ##-1 Configure the FDCAN filters ########################################

	// Configure Rx filter
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 0;
	sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 0x321;
	sFilterConfig.FilterID2 = 0x7FF;
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

	/* Configure global filter:
	 Filter all remote frames with STD and EXT ID
	 Reject non matching frames with STD ID and EXT ID */
	if (HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT,
	FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
	{
		Error_Handler();
	}

	/*##-2 Start FDCAN controller (continuous listening CAN bus) ##############*/
	if (HAL_FDCAN_Start(hfdcan) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0)
			!= HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0)
			!= HAL_OK)
	{
		Error_Handler();
	}
}

void can_controller_t::send_message(uint8_t message)
{
	/*##-3 Transmit messages ##################################################*/
	/* Add message to Tx FIFO */
	TxData[0] = 0x10;
	TxData[1] = 0x32;
	TxData[2] = 0x54;
	TxData[3] = 0x76;
	TxData[4] = 0x98;
	TxData[5] = 0x00;
	TxData[6] = 0x11;
	TxData[7] = 0x22;

	TxHeader.Identifier = 0x321;
	TxHeader.IdType = FDCAN_STANDARD_ID;
	TxHeader.TxFrameType = FDCAN_DATA_FRAME;
	TxHeader.DataLength = FDCAN_DLC_BYTES_8;
	TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
	TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
	TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	TxHeader.MessageMarker = 0;

	if (HAL_FDCAN_AddMessageToTxFifoQ(this->hfdcan, &TxHeader, TxData)
			!= HAL_OK)
	{
		Error_Handler();
	}
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
		if (HAL_FDCAN_GetRxMessage(this->hfdcan, FDCAN_RX_FIFO0, &rx_can_message.RxHeader,
				rx_can_message.RxData) != HAL_OK)
		{
			Error_Handler();
		}

		this->rx_buf_0.push(rx_can_message);
	}
}

void can_controller_t::rx_fifo_1_callback(uint32_t RxFifo1ITs)
{
}

// ##################################################################
//    Error
// ##################################################################

void can_controller_t::error_callback()
{
}
