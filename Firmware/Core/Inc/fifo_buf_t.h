/**
 *	\file fifo_buf_t.h
 *	\author Richard Carter
 *  \date 12 Nov 2020
 *	\brief Generic Template FIFO Buffer Class File
 */

#ifndef SRC_FIFO_BUF_T_H_
#define SRC_FIFO_BUF_T_H_

#include <cstdint>

/**
 * \class fifo_buf_t
 * \brief Generic Template FIFO Class
 *
 * Provides standard FIFO queue methods to add and remove data
 * from the buffer
 */
template<class T, int SIZE>
class fifo_buf_t {
public:
	/**
	 * \param full_keep_new Choose to throw away old or new data when overflowing
	 * \brief Construct the FIFO buffer and initialise the default to 0
	 */
	fifo_buf_t(bool full_keep_new = false);

	/**
	 * \brief Destruct the FIFO buffer
	 */
	virtual ~fifo_buf_t();

	/**
	 * \return Buffer is full
	 * \brief Checks if the buffer is full
	 */
	bool full();

	/**
	 * \return Buffer is empty
	 * \brief Checks if the buffer is empty
	 */
	bool empty();

	/**
	 * \return Number of elements in buffer
	 * \brief Returns how many elements are in the buffer
	 */
	uint16_t count();

	/**
	 * \param val Pointer to T to read to
	 * \return Returns success or failure
	 * \brief Reads a T from the buffer into val (if not empty) and
	 * 		removes the element from the FIFO
	 */
	bool pop(T* val);

	/**
	 * \param val Pointer to T to read to
	 * \param pos Position of the element to read (0 = oldest element)
	 * \return Returns success or failure
	 * \brief Reads a T from the buffer into val (if the position is less than
	 * 		than the number of elements in the buffer)
	 */
	bool read(T* val, uint16_t pos);

	/**
	 * \param val Pointer to T to read to
	 * \return Returns success or failure
	 * \brief Reads the oldest element in the buffer (if not empty)
	 */
	bool oldest(T* val);

	/**
	 * \param val Pointer to T to read to
	 * \return Returns success or failure
	 * \brief Reads the newest element in the buffer (if not empty)
	 */
	bool newest(T* val);

	/**
	 * \param val T to write to the buffer
	 * \return Returns success or failure
	 * \brief Writes the provided T into the buffer provided the FIFO isn't
	 * full or is keeping new values when overflowing
	 */
	bool push(T val);
private:
	/**
	 * \brief Whether to keep or discard old data when overflowing
	 */
	bool full_keep_new;

	/**
	 * \brief Index in the array representing the newest value in the FIFO
	 */
	volatile uint16_t head;

	/**
	 * \brief Index-1 in the array representing the oldest value in the FIFO
	 */
	volatile uint16_t tail;

	/**
	 * \brief How many elements there are in the array
	 */
	volatile uint16_t element_count;

	/**
	 * \brief Underlying array to store the elements on the buffer
	 */
	T buffer[SIZE];
};

#endif /* SRC_FIFO_BUF_T_H_ */
