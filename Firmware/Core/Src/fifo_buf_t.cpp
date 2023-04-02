/**
 *	\file fifo_buf_t.cpp
 *	\author Richard Carter
 *  \date 12 Nov 2020
 */

#include "fifo_buf_t.h"

template <class T>
fifo_buf_t<T>::fifo_buf_t(bool full_keep_new) : full_keep_new(full_keep_new), head(0), tail(0), element_count(0)
{
	for (uint16_t i = 0; i < FIFO_BUFFER_SIZE; i++)
	{
		this->buffer[i] = 0;
	}
}

template <class T>
fifo_buf_t<T>::~fifo_buf_t()
{
}

template <class T>
bool fifo_buf_t<T>::full()
{
	return this->element_count == FIFO_BUFFER_SIZE;
}

template <class T>
bool fifo_buf_t<T>::empty()
{
	return this->element_count == 0;
}

template <class T>
uint16_t fifo_buf_t<T>::count()
{
	return this->element_count;
}

template <class T>
bool fifo_buf_t<T>::pop(T* val)
{
	if (this->empty())
	{
		return false;
	}
	else
	{
		this->element_count--;

		uint16_t newTail = this->tail + 1;
		if (newTail >= FIFO_BUFFER_SIZE)
			newTail -= FIFO_BUFFER_SIZE;
		this->tail = newTail;
		(*val) = this->buffer[this->tail];
		return true;
	}
}

template <class T>
bool fifo_buf_t<T>::read(T* val, uint16_t pos)
{
	if (pos >= this->element_count)
	{
		return false;
	}
	else
	{
		// 0 is the oldest value
		uint16_t indexOfPosition = this->tail + pos + 1;
		if (indexOfPosition >= FIFO_BUFFER_SIZE)
			indexOfPosition -= FIFO_BUFFER_SIZE;
		(*val) = this->buffer[indexOfPosition];
		return true;
	}
}

template <class T>
bool fifo_buf_t<T>::oldest(T* val)
{
	return this->read(val, 0);
}

template <class T>
bool fifo_buf_t<T>::newest(T* val)
{
	return this->read(val, this->element_count - 1);
}

template <class T>
bool fifo_buf_t<T>::push(T val)
{
	if (this->full() && !this->full_keep_new)
	{
		return false;
	}
	else
	{
		if (this->full())
		{
			this->pop(NULL);
		}

		uint16_t newHead = this->head + 1;
		if (newHead >= FIFO_BUFFER_SIZE)
			newHead -= FIFO_BUFFER_SIZE;
		this->head = newHead;
		this->buffer[this->head] = val;

		this->element_count++;
		return true;
	}
}

template class fifo_buf_t<uint8_t>;
