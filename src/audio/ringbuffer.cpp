// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the RingBuffer class.
 */

#include <cassert>
#include <atomic>
#include <mutex>
#include <vector>

#include "../gsl/gsl"
#include "../errors.hpp"
#include "../messages.h"
#include "ringbuffer.hpp"

/* Assumptions:

   1) single producer, single consumer, enforced by locks.
      - when reading, count can only increase (read capacity can only increase)
      - when writing, count can only decrease (write capacity can only increase)
      - only the reader can move the read pointer, and it may do so non-atomically
      - only the writer can move the write pointer, and it may do so non-atomically
   2) capacities always underestimate
      - when reading, ATOMICALLY decrease count (increase read capacity) AFTER read
                      read capacity may be lower than actual
      - when writing, ATOMICALLY increase count (read capacity) AFTER write
                      write capacity may be lower than actual
      - always atomically read capacities */

RingBuffer::RingBuffer(int power, int size)
{
	if (power <= 0) throw InternalError("ringbuffer power must be positive");
	if (size <= 0) throw InternalError("ringbuffer element size must be positive");

    this->buffer = std::vector<uint8_t>((1 << power) * size);
    this->r_it = this->buffer.cbegin();
    this->w_it = this->buffer.begin();
    this->el_size = size;

    // Only one thread can hold this at the moment, so we're ok to
    // do this without locks.
	this->FlushInner();
}

RingBuffer::~RingBuffer()
{
}

inline size_t RingBuffer::ReadCapacity() const
{
	/* Acquire order here means two things:
	*
	* 1) no other loads in the thread checking WriteCapacity (ie the
	*    producer) can be ordered before it;
	* 2) this load sees all 'release' writes in other threads (usually
	*    consumers increasing the write capacity).
	*/
	return this->count.load(std::memory_order_acquire);
}

inline size_t RingBuffer::WriteCapacity() const
{
	return this->buffer.size()/this->el_size - ReadCapacity();
}

unsigned long RingBuffer::Write(const char *start, size_t count)
{
	/* Acquire write lock to make sure only one write can occur at a given time,
	 * and also that we can't be flushed in the middle of writing.
	 *
	 * This ensures that the write capacity can only go up from when we
	 * read it here.
	 */
	std::lock_guard<std::mutex> w_guard(this->w_lock);

	if (count == 0) throw InternalError("tried to store 0 items in ringbuffer");

	/* Remember, this is pessimistic:
	 * the write capacity can be increased after this point by a consumer.
	 */
	auto write_capacity_estimate = WriteCapacity();
	if (write_capacity_estimate < count) throw InternalError("ringbuffer overflow");

	auto to_write = std::min(write_capacity_estimate, count);

	/* At this stage, we're the only thread that can be accessing this part of
	 * the buffer, so we can proceed non-atomically.  The release we do at the
	 * end makes our changes available to the consumer.
	 */
	auto write_bytes = to_write * this->el_size;
	// Ringbuffers loop, so how many bytes can we store until we have to loop?
	auto bytes_to_end = distance(this->w_it, this->buffer.end());
	// Make sure we got the iterators the right way round.
	assert(0 <= bytes_to_end);
	
	auto write_end_bytes = std::min(write_bytes, static_cast<size_t>(bytes_to_end));
	this->w_it = copy_n(start, write_end_bytes, this->w_it);
	
	// Do we need to loop?  If so, do that.
	auto write_start_bytes = write_bytes - write_end_bytes;
	if (0 < write_start_bytes) {
		Expects(this->w_it == this->buffer.end());
		this->w_it = copy_n(start + write_end_bytes, write_start_bytes, this->buffer.begin());
		Ensures(this->w_it > this->buffer.begin());
	}

	/* Now tell the consumer it can read some more data (this HAS to be done here,
	 * to avoid the consumer over-reading).
	 * 
	 * Because the other thread might have moved count since we last checked it,
	 * this needs to be acquire-release.
	 */
	this->count.fetch_add(to_write, std::memory_order_acq_rel);

	Ensures(write_start_bytes + write_end_bytes == write_bytes);
	return to_write;
}

size_t RingBuffer::Read(char *start, size_t count)
{
	/* Acquire read lock to make sure only one read can occur at a given time,
	 * and also that we can't be flushed in the middle of reading.
	 *
	 * This ensures that the read capacity can only go up from when we
	 * read it here.
	 */
	std::lock_guard<std::mutex> r_guard(this->r_lock);

	if (count == 0) throw InternalError("tried to take 0 items from ringbuffer");

	/* Remember, this is pessimistic:
	 * the read capacity can be increased after this point by a producer.
	 */
	auto read_capacity_estimate = ReadCapacity();
	if (read_capacity_estimate < count) throw InternalError("ringbuffer underflow");

	auto to_read = std::min(read_capacity_estimate, count);

	/* At this stage, we're the only thread that can be accessing this part of
 	 * the buffer, so we can proceed non-atomically.  The release we do at the
	 * end makes our changes available to the consumer.
	 */
	auto read_bytes = to_read * this->el_size;
	// Ringbuffers loop, so how many bytes can we read until we have to loop?
	auto bytes_to_end = distance(this->r_it, this->buffer.cend());
	// Make sure we got the iterators the right way round.
	assert(0 <= bytes_to_end);

	auto read_end_bytes = std::min(read_bytes, static_cast<size_t>(bytes_to_end));
	start = copy_n(this->r_it, read_end_bytes, start);
	this->r_it += read_end_bytes;

	// Do we need to loop?  If so, do that.
	auto read_start_bytes = read_bytes - read_end_bytes;
	if (read_start_bytes > 0) {
		Expects(this->r_it == this->buffer.cend());
		copy_n(this->buffer.cbegin(), read_start_bytes, start);
		this->r_it = this->buffer.cbegin() + read_start_bytes;
		Ensures(this->r_it > this->buffer.cbegin());
	}

	/* Now tell the producer it can write some more data (this HAS to be done here,
	 * to avoid the producer over-writing).
	 *
	 * Because the other thread might have moved count since we last checked it,
	 * this needs to be acquire-release.
	 */
	if (this->count.fetch_sub(to_read, std::memory_order_acq_rel) < to_read)
		throw InternalError("capacity decreased unexpectedly");

	Ensures(read_start_bytes + read_end_bytes == read_bytes);
	return to_read;
}

void RingBuffer::Flush()
{
	// Make sure nothing is reading or writing.
	std::lock_guard<std::mutex> r_guard(this->r_lock);
	std::lock_guard<std::mutex> w_guard(this->w_lock);

	FlushInner();

	Ensures(this->ReadCapacity() == 0);
}

inline void RingBuffer::FlushInner()
{
	this->count.store(0, std::memory_order_release);
}