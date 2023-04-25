/*
 * CanController.cpp
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#include <can_controller_t.h>

#include "uart_controllers.h"
#include "main.h"

#include <cstdio>
#include <cstring>

extern uart_controller_t uart_controller_1;

#ifdef DEBUG
char buff1[256];
#endif

template<int SIZE0, int SIZE1>
can_controller_t<SIZE0, SIZE1>::can_controller_t(GPIO_TypeDef *s_GPIO_Port,
		uint16_t s_Pin) :
		S_GPIO_Port(s_GPIO_Port), S_Pin(s_Pin), hfdcan(NULL), rx_buf_0(
				fifo_buf_t<rx_can_message_t, SIZE0>(false)), // Configured to discard new data when full
		rx_buf_1(fifo_buf_t<rx_can_message_t, SIZE1>(false)) // Configured to discard new data when full
{
}

template<int SIZE0, int SIZE1>
can_controller_t<SIZE0, SIZE1>::~can_controller_t()
{
	// TODO Auto-generated destructor stub
}

template<int SIZE0, int SIZE1>
void can_controller_t<SIZE0, SIZE1>::init(FDCAN_HandleTypeDef *hfdcan)
{
	this->hfdcan = hfdcan;

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

template<int SIZE0, int SIZE1>
void can_controller_t<SIZE0, SIZE1>::silence(bool enable)
{
	if (enable)
		HAL_GPIO_WritePin(this->S_GPIO_Port, this->S_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(this->S_GPIO_Port, this->S_Pin, GPIO_PIN_RESET);

}

template<int SIZE0, int SIZE1>
bool can_controller_t<SIZE0, SIZE1>::send_message(
		FDCAN_TxHeaderTypeDef *txHeader, uint8_t *txData)
{
	if (HAL_FDCAN_GetTxFifoFreeLevel(this->hfdcan) > 0)
	{
		if (HAL_FDCAN_AddMessageToTxFifoQ(this->hfdcan, txHeader, txData)
				!= HAL_OK)
			Error_Handler();
		return true;
	}
	else
		return false;
}

template<int SIZE0, int SIZE1>
bool can_controller_t<SIZE0, SIZE1>::send_copy_of_rx_message(
		rx_can_message_t *rx_can_message)
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

	if (HAL_FDCAN_GetTxFifoFreeLevel(this->hfdcan) > 0)
	{
		if (HAL_FDCAN_AddMessageToTxFifoQ(this->hfdcan, &TxHeader,
				rx_can_message->RxData) != HAL_OK)
		{
			//uint32_t err = HAL_FDCAN_GetError(this->hfdcan);
		}
		return true;
	}
	else
		return false;
}

template<int SIZE0, int SIZE1>
bool can_controller_t<SIZE0, SIZE1>::read_message_0(
		rx_can_message_t *rx_can_message)
{
	bool success = this->rx_buf_0.oldest(rx_can_message);
	return success;
}

template<int SIZE0, int SIZE1>
bool can_controller_t<SIZE0, SIZE1>::pop_message_0()
{
	// Ensure interrupt doesn't occur while buffer is being changed
	HAL_NVIC_DisableIRQ(TIM16_FDCAN_IT0_IRQn);
	bool success = this->rx_buf_0.pop();
	HAL_NVIC_EnableIRQ(TIM16_FDCAN_IT0_IRQn);
	return success;
}

template<int SIZE0, int SIZE1>
bool can_controller_t<SIZE0, SIZE1>::read_message_1(
		rx_can_message_t *rx_can_message)
{
	bool success = this->rx_buf_1.oldest(rx_can_message);
	return success;
}

template<int SIZE0, int SIZE1>
bool can_controller_t<SIZE0, SIZE1>::pop_message_1()
{
	// Ensure interrupt doesn't occur while buffer is being changed
	HAL_NVIC_DisableIRQ(TIM16_FDCAN_IT0_IRQn);
	bool success = this->rx_buf_0.pop();
	HAL_NVIC_EnableIRQ(TIM16_FDCAN_IT0_IRQn);
	return success;
}

template<int SIZE0, int SIZE1>
void can_controller_t<SIZE0, SIZE1>::rx_fifo_0_callback(uint32_t RxFifo0ITs)
{
	if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
	{
		rx_can_message_t rx_can_message;

		/* Retrieve Rx messages from RX FIFO0 */
		if (HAL_FDCAN_GetRxMessage(this->hfdcan, FDCAN_RX_FIFO0,
				&rx_can_message.RxHeader, rx_can_message.RxData) != HAL_OK)
			Error_Handler();

		if (!this->rx_buf_0.push(rx_can_message))
		{
#ifdef DEBUG
			uart_controller_1.write_bytes("CAN FIFO0 full\r\n", 16);
#endif
		}
	}
}

template<int SIZE0, int SIZE1>
void can_controller_t<SIZE0, SIZE1>::rx_fifo_1_callback(uint32_t RxFifo1ITs)
{
	if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET)
	{
		rx_can_message_t rx_can_message;

		/* Retrieve Rx messages from RX FIFO1 */
		if (HAL_FDCAN_GetRxMessage(this->hfdcan, FDCAN_RX_FIFO1,
				&rx_can_message.RxHeader, rx_can_message.RxData) != HAL_OK)
			Error_Handler();

		if (!this->rx_buf_1.push(rx_can_message))
		{
#ifdef DEBUG
			uart_controller_1.write_bytes("CAN FIFO1 full\r\n", 16);
#endif
		}
	}
}

// ##################################################################
//    Error
// ##################################################################

template<int SIZE0, int SIZE1>
void can_controller_t<SIZE0, SIZE1>::error_callback()
{
	//uint32_t err = HAL_FDCAN_GetError(this->hfdcan);
}

template class can_controller_t<512, 4> ;
