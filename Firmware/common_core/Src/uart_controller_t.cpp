/*
 * uart_controller_t.cpp
 *
 *  Created on: Mar 19, 2023
 *      Author: richa
 */

#include <uart_controller_t.h>

#include "main.h"

uart_controller_t::uart_controller_t() :
		huart(NULL),
		rx_buf(fifo_buf_t<uint8_t, 512>(false)),
		tx_buf(fifo_buf_t<uint8_t, 512>(false)),
		transmitting_byte(0),
		receiving_byte(0)
{
}

uart_controller_t::~uart_controller_t()
{
}

void uart_controller_t::init(UART_HandleTypeDef* huart, IRQn_Type irqn)
{
	this->huart = huart;
	this->irqn = irqn;

	HAL_UART_Receive_IT(this->huart, &this->receiving_byte, 1);
}



// ##################################################################
//    Writing
// ##################################################################

uint8_t uart_controller_t::write_bytes(uint8_t * buf, uint8_t len)
{
	uint8_t i = 0;
	while (i < len && this->write_byte(buf[i]))
		i++;
	return i;
}

uint8_t uart_controller_t::write_bytes(const char* buf, uint8_t len)
{
	return this->write_bytes((uint8_t*)buf, len);
}

uint8_t uart_controller_t::write_bytes(const char* buf)
{
	uint8_t len = 0;
	while (len < 200 && buf[len] != '\0')
		len++;
	return this->write_bytes(buf, len);
}

uint8_t uart_controller_t::write_tx_fifo_level()
{
	uint16_t tx_fifo_level = this->tx_buf.count();
	uint8_t transmission_array[2];
	transmission_array[0] = (tx_fifo_level & 0xFF00) >> 8;
	transmission_array[1] = (tx_fifo_level & 0x00FF);
	return this->write_bytes(transmission_array, 2);
}

bool uart_controller_t::write_byte(uint8_t byte)
{
	// Ensure interrupt doesn't occur while buffer is being changed
    HAL_NVIC_DisableIRQ(this->irqn);
	bool success = this->tx_buf.push(byte);
    HAL_NVIC_EnableIRQ(this->irqn);

	if (huart->gState == HAL_UART_STATE_READY)
	{
		this->transmitting_byte = byte;
		HAL_UART_Transmit_IT(this->huart, &this->transmitting_byte, 1);
	}

	return success;
}

void uart_controller_t::tx_cplt_callback()
{
	// Called by an ISR, needs to be quick
	this->tx_buf.pop();
	uint8_t next_byte_to_transmit;
	if (this->tx_buf.oldest(&next_byte_to_transmit))
	{
		this->transmitting_byte = next_byte_to_transmit;
		HAL_UART_Transmit_IT(huart, &this->transmitting_byte, 1);
	}
}

// ##################################################################
//    Reading
// ##################################################################


bool uart_controller_t::read_byte(uint8_t * byte)
{
	// Ensure interrupt doesn't occur while buffer is being changed
    HAL_NVIC_DisableIRQ(this->irqn);
	bool success = this->rx_buf.pop(byte);
    HAL_NVIC_EnableIRQ(this->irqn);
    return success;
}

uint8_t uart_controller_t::read_bytes(uint8_t *buf, uint8_t len)
{
	uint8_t i = 0;
	while (i < len && this->read_byte(&buf[i]))
		i++;
	return i;
}

void uart_controller_t::rx_cplt_callback()
{
	// Called by an ISR, needs to be quick
	this->rx_buf.push(this->receiving_byte);
	__HAL_UART_FLUSH_DRREGISTER(huart);
	HAL_UART_Receive_IT(huart, &this->receiving_byte, 1);
}

// ##################################################################
//    Error
// ##################################################################

void uart_controller_t::error_callback()
{
}
