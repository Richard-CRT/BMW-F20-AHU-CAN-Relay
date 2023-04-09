/*
 * CanController.h
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#ifndef CAN_CONTROLLER_T_H_
#define CAN_CONTROLLER_T_H_

#include "stm32g0xx_hal.h"

#include "fifo_buf_t.h"
#include "rx_can_message_t.h"

class can_controller_t
{
public:
	can_controller_t(GPIO_TypeDef *S_GPIO_Port, uint16_t S_Pin);
	virtual ~can_controller_t();

	void init(FDCAN_HandleTypeDef *hfdcan);

	void silence(bool enable);

	/**
	 * \param byte Message
	 * \brief
	 */
	void send_message(FDCAN_TxHeaderTypeDef *txHeader, uint8_t *txData);

	/**
	 * \param byte Message
	 * \brief
	 */
	void send_copy_of_rx_message(rx_can_message_t *rx_can_message);

	/**
	 * \param byte Pointer to rx_can_message to return the read value to
	 * \return Success/Failure to read a rx_can_message from the receiving buffer
	 * \brief Non-blocking read a byte from the receiving buffer 0
	 */
	bool read_message_0(rx_can_message_t *rx_can_message);
	/**
	 * \param byte Pointer to rx_can_message to return the read value to
	 * \return Success/Failure to read a rx_can_message from the receiving buffer
	 * \brief Non-blocking read a byte from the receiving buffer 1
	 */
	bool read_message_1(rx_can_message_t *rx_can_message);

	void rx_fifo_0_callback(uint32_t RxFifo0ITs);
	void rx_fifo_1_callback(uint32_t RxFifo1ITs);

	/**
	 * \brief UART Error Interrupt Callback
	 *
	 */
	void error_callback();
private:
	GPIO_TypeDef *S_GPIO_Port;
	uint16_t S_Pin;
	FDCAN_HandleTypeDef *hfdcan;
	FDCAN_FilterTypeDef sFilterConfig;

	/**
	 * \brief FIFO Queue to buffer incoming bytes
	 *
	 * Configured to keep old data when overflowing
	 * */
	fifo_buf_t<rx_can_message_t, 256> rx_buf_0;
	fifo_buf_t<rx_can_message_t, 256> rx_buf_1;
};

#endif /* CAN_CONTROLLER_T_H_ */
