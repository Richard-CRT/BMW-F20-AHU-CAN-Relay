/**
 *	\file fifo_buf_t.cpp
 *	\author Richard Carter
 *  \date 12 Nov 2020
 */

#include "fifo_buf_t.h"

#include "main.h"
#include "rx_can_message_t.h"

template<class T, int SIZE>
fifo_buf_t<T, SIZE>::fifo_buf_t(bool full_keep_new) :
		full_keep_new(full_keep_new), head(0), tail(0), element_count(0)
{
	for (uint16_t i = 0; i < SIZE; i++)
	{
		this->buffer[i] = T();
	}
}

template<class T, int SIZE>
fifo_buf_t<T, SIZE>::~fifo_buf_t()
{
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::full()
{
	return this->element_count == SIZE;
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::empty()
{
	return this->element_count == 0;
}

template<class T, int SIZE>
uint16_t fifo_buf_t<T, SIZE>::count()
{
	return this->element_count;
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::pop()
{
	if (this->empty())
	{
		return false;
	}
	else
	{
		this->element_count--;

		uint16_t newTail = this->tail + 1;
		if (newTail >= SIZE)
			newTail -= SIZE;
		this->tail = newTail;
		return true;
	}
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::pop(T *val)
{
	bool result = this->pop();
	if (result)
		(*val) = this->buffer[this->tail];
	return result;
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::read(T *val, uint16_t pos)
{
	if (pos >= this->element_count)
	{
		return false;
	}
	else
	{
		// 0 is the oldest value
		uint16_t indexOfPosition = this->tail + pos + 1;
		if (indexOfPosition >= SIZE)
			indexOfPosition -= SIZE;
		(*val) = this->buffer[indexOfPosition];
		return true;
	}
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::oldest(T *val)
{
	return this->read(val, 0);
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::newest(T *val)
{
	return this->read(val, this->element_count - 1);
}

template<class T, int SIZE>
bool fifo_buf_t<T, SIZE>::push(T val)
{
	if (this->full() && !this->full_keep_new)
		return false;
	else
	{
		if (this->full())
			this->pop();

		uint16_t newHead = this->head + 1;
		if (newHead >= SIZE)
			newHead -= SIZE;
		this->head = newHead;
		this->buffer[this->head] = val;

		this->element_count++;
		return true;
	}
}

template class fifo_buf_t<uint8_t, 512> ;
template class fifo_buf_t<rx_can_message_t, 512> ;
template class fifo_buf_t<rx_can_message_t, 4> ;
