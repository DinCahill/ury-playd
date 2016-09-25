// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the RingBuffer class.
 */

#include <cassert>
#include <atomic>
#include <vector>

extern "C" {
#include "../contrib/pa_ringbuffer/pa_ringbuffer.h"
}

#include "../gsl/gsl"
#include "../errors.hpp"
#include "../messages.h"
#include "ringbuffer.hpp"

/* Assumptions:

   1) single producer, single consumer
      - when reading, read capacity can only increase, write capacity never changes
      - when writing, write capacity can only increase, write capacity never changes
      - only the reader can move the read pointer, and it may do so non-atomically
      - only the writer can move the write pointer, and it may do so non-atomically
   2) capacities always underestimate
      - when reading, ATOMICALLY decrease read capacity BEFORE read (but after commit)
                      ATOMICALLY increase write capacity AFTER read
                      read capacity may be lower than actual
      - when writing, ATOMICALLY decrease write capacity BEFORE write (but after commit)
                      ATOMICALLY increase read capacity AFTER write
                      write capacity may be lower than actual
      - always atomically read capacities */

RingBuffer::RingBuffer(int power, int size)
{
	if (power <= 0) throw InternalError("ringbuffer power must be positive");
	if (size <= 0) throw InternalError("ringbuffer element size must be positive");

    this->buffer = std::vector<std::uint8_t>((1 << power) * size);
    this->r_it = this->buffer.cbegin();
    this->w_it = this->buffer.begin();
    this->el_size = size;
    
    // Only one thread can hold this at the moment, so we're ok to
    // do this non-atomically.
    this->r_count = 0;
    this->w_count = this->buffer.size() / this->el_size;
}

RingBuffer::~RingBuffer()
{
}

size_t RingBuffer::WriteCapacity() const
{
    return this->w_count.load()
    
	return CountCast(PaUtil_GetRingBufferWriteAvailable(this->rb));
}

size_t RingBuffer::ReadCapacity() const
{
	return CountCast(PaUtil_GetRingBufferReadAvailable(this->rb));
}

unsigned long RingBuffer::Write(const char *start, unsigned long count)
{
	if (count == 0) throw InternalError("tried to store 0 items in ringbuffer");
	if (WriteCapacity() < count) throw InternalError("ringbuffer overflow");

	return CountCast(PaUtil_WriteRingBuffer(
	        this->rb, start, static_cast<ring_buffer_size_t>(count)));
}

unsigned long RingBuffer::Read(char *start, unsigned long count)
{
	if (count == 0) throw InternalError("tried to read 0 items from ringbuffer");
	if (ReadCapacity() < count) throw InternalError("ringbuffer underflow");

	return CountCast(PaUtil_ReadRingBuffer(
	        this->rb, start, static_cast<ring_buffer_size_t>(count)));
}

void RingBuffer::Flush()
{
	PaUtil_FlushRingBuffer(this->rb);
	assert(this->ReadCapacity() == 0);
}

/* static */ unsigned long RingBuffer::CountCast(ring_buffer_size_t count)
{
	return static_cast<unsigned long>(count);
}
