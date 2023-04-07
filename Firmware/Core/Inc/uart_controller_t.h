/*
 * uart_controller_t.h
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#ifndef UART_CONTROLLER_T_H_
#define UART_CONTROLLER_T_H_

#include "stm32g0xx_hal.h"

#include "fifo_buf_t.h"

class uart_controller_t {
public:
	uart_controller_t();
	virtual ~uart_controller_t();

	void init(UART_HandleTypeDef* huart, IRQn_Type irqn);

	/**
	 * \param buf Pointer to byte buffer to transmit
	 * \param len How many bytes in the buffer
	 * \return How many bytes were successfully added to transmitting FIFO buffer
	 * \brief Non-blocking write a buffer of bytes to the UART transmitting buffer
	 */
	uint8_t write_bytes(uint8_t * buf, uint8_t len);

	/**
	 * \param buf Pointer to character buffer to transmit
	 * \param len How many characters in the buffer
	 * \return How many characters were successfully added to transmitting FIFO buffer
	 * \brief Non-blocking write a string to the UART transmitting buffer
	 */
	uint8_t write_bytes(const char * buf, uint8_t len);

	/**
	 * \param buf Pointer to character buffer to transmit
	 * \return How many characters were successfully added to transmitting FIFO buffer
	 * \brief Non-blocking write a string to the UART transmitting buffer
	 */
	uint8_t write_bytes(const char * buf);

	/**
	 * \param byte Byte to transmit
	 * \return Success/Failure to add byte to transmit buffer
	 * \brief Non-blocking write a byte to the UART transmitting buffer
	 */
	bool write_byte(uint8_t byte);

	/**
	 * \return How many characters were successfully added to transmitting FIFO buffer
	 * \brief Non-blocking write the current level of the UART transmitting buffer to
	 * 		the UART transmitting buffer
	 */
	uint8_t write_tx_fifo_level();

	/**
	 * \param byte Pointer to byte to return the read value to
	 * \return Success/Failure to read a byte from the receiving buffer
	 * \brief Non-blocking read a byte from the UART receiving buffer
	 */
	bool read_byte(uint8_t * byte);

	/**
	 * \param buf Pointer to character buffer to receive into
	 * \param len How many bytes to read
	 * \return How many bytes were successfully read
	 * \brief Non-blocking read bytes from the UART receiving buffer
	 */
	uint8_t read_bytes(uint8_t * buf, uint8_t len);

	/**
	 * \brief UART Transmit Complete Interrupt Callback
	 */
	void tx_cplt_callback();

	/**
	 * \brief UART Receive Complete Interrupt Callback
	 */
	void rx_cplt_callback();

	/**
	 * \brief UART Error Interrupt Callback
	 *
	 */
	void error_callback();

private:
	UART_HandleTypeDef* huart;
	IRQn_Type irqn;

	/**
	 * \brief FIFO Queue to buffer incoming bytes
	 *
	 * Configured to keep old data when overflowing
	 * */
	fifo_buf_t<uint8_t> rx_buf;

	/**
	 * \brief FIFO Queue to buffer outgoing bytes
	 *
	 * Configured to keep old data when overflowing
	 * */
	fifo_buf_t<uint8_t> tx_buf;

	/**
	 * \brief Storage of the byte currently being transmitted
	 *
	 * Prevents the variable from going out of scope while the chip is transmitting
	 */
	uint8_t transmitting_byte;

	/**
	 * \brief Storage of the byte currently being received
	 *
	 * Prevents the variable from going out of scope while the chip is receiving
	 */
	uint8_t receiving_byte;
};

#endif /* UART_CONTROLLER_T_H_ */
