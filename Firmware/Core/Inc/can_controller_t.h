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

class can_controller_t {
public:
	can_controller_t();
	virtual ~can_controller_t();

	void init(FDCAN_HandleTypeDef* hfdcan);

	/**
	 * \param byte Message
	 * \brief
	 */
	void send_message(uint8_t message);

	/**
	 * \param byte Pointer to rx_can_message to return the read value to
	 * \return Success/Failure to read a rx_can_message from the receiving buffer
	 * \brief Non-blocking read a byte from the receiving buffer 0
	 */
	bool read_message_0(rx_can_message_t* rx_can_message);
	/**
	 * \param byte Pointer to rx_can_message to return the read value to
	 * \return Success/Failure to read a rx_can_message from the receiving buffer
	 * \brief Non-blocking read a byte from the receiving buffer 1
	 */
	bool read_message_1(rx_can_message_t* rx_can_message);

	void rx_fifo_0_callback(uint32_t RxFifo0ITs);
	void rx_fifo_1_callback(uint32_t RxFifo1ITs);

	/**
	 * \brief UART Error Interrupt Callback
	 *
	 */
	void error_callback();
private:
	FDCAN_HandleTypeDef* hfdcan;
	FDCAN_FilterTypeDef sFilterConfig;
	FDCAN_TxHeaderTypeDef TxHeader;
	uint8_t TxData[8];

	/**
	 * \brief FIFO Queue to buffer incoming bytes
	 *
	 * Configured to keep new data when overflowing
	 * */
	fifo_buf_t<rx_can_message_t> rx_buf_0;
	fifo_buf_t<rx_can_message_t> rx_buf_1;
};

#endif /* CAN_CONTROLLER_T_H_ */
