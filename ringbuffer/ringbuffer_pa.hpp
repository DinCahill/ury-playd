// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * The PaRingBuffer class template.
 * @see ringbuffer/ringbuffer.hpp
 * @see ringbuffer/ringbuffer_boost.hpp
 */

#ifndef PS_RINGBUFFER_PA_HPP
#define PS_RINGBUFFER_PA_HPP

#include <cassert>

extern "C" {
#include "../contrib/pa_ringbuffer.h"
}

#include "../errors.hpp"
#include "../messages.h"

#include "ringbuffer.hpp"

/**
 * Implementation of RingBuffer using the PortAudio C ring buffer.
 *
 * This is stable and performs well, but, as it is C code, necessitates some
 * hoop jumping to integrate and could do with being replaced with a native
 * solution.
 *
 * The PortAudio ring buffer is provided in the contrib/ directory.
 */
template <typename T1, typename T2>
class PaRingBuffer : public RingBuffer<T1, T2> {
public:
	/**
	 * Constructs a PaRingBuffer.
	 * @param power n, where 2^n is the number of elements in the ring buffer.
	 * @param size The size of one element in the ring buffer.
	 */
	PaRingBuffer(int power, int size)
	{
		this->rb = new PaUtilRingBuffer;
		this->buffer = new char[(1 << power) * size];

		int init_result = PaUtil_InitializeRingBuffer(
		                this->rb, size,
		                static_cast<ring_buffer_size_t>(1 << power),
		                this->buffer);
		if (init_result != 0) {
			throw InternalError(MSG_OUTPUT_RINGINIT);
		}
	}

	/**
	 * Destructs a PaRingBuffer.
	 */
	~PaRingBuffer()
	{
		assert(this->rb != nullptr);
		delete this->rb;

		assert(this->buffer != nullptr);
		delete[] this->buffer;
	}

	T2 WriteCapacity() const override
	{
		return CountCast(PaUtil_GetRingBufferWriteAvailable(this->rb));
	}

	T2 ReadCapacity() const override
	{
		return CountCast(PaUtil_GetRingBufferReadAvailable(this->rb));
	}

	T2 Write(T1 *start, T2 count) override
	{
		return CountCast(PaUtil_WriteRingBuffer(
		                this->rb, start,
		                static_cast<ring_buffer_size_t>(count)));
	}

	T2 Read(T1 *start, T2 count) override
	{
		return CountCast(PaUtil_ReadRingBuffer(
		                this->rb, start,
		                static_cast<ring_buffer_size_t>(count)));
	}

	void Flush() override
	{
		PaUtil_FlushRingBuffer(this->rb);
	}

private:
	char *buffer;         ///< The array used by the ringbuffer.
	PaUtilRingBuffer *rb; ///< The internal PortAudio ringbuffer.

	/**
	 * Converts a ring buffer size into an external size.
	 * @param count  The size/count in PortAudio form.
	 * @return       The size/count after casting to T2.
	 */
	T2 CountCast(ring_buffer_size_t count) const
	{
		return static_cast<T2>(count);
	}
};

#endif // PS_RINGBUFFER_PA_HPP
