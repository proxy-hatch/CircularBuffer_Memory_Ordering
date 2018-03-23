/* CircBuf - A fast, thread-safe, lockless circular buffer. */
/* read()/write() interface adjusted by Craig Scratchley to be similar
 * to the posix read() and write() functions in order to increase efficiency.
 * Craig Scratchley -- March 29 2011 */

#ifndef RAGE_UTIL_CIRCULAR_BUFFER
#define RAGE_UTIL_CIRCULAR_BUFFER

#include <cstring>
#include <atomic>

/* Lock-free circular buffer.  This should be threadsafe if one thread is reading
 * and another is writing. */
template<class T>
class CircBuf {
	T *buf;
	/* read_pos is the position data is read from; write_pos is the position
	 * data is written to.  If read_pos == write_pos, the buffer is empty.
	 *
	 * There will always be at least one position empty, as a completely full
	 * buffer (read_pos == write_pos) is indistinguishable from an empty buffer.
	 *
	 * Invariants: read_pos < size, write_pos < size. */
	unsigned size;
	unsigned m_iBlockSize;

	// Craig says:  making the variables volatile won't make this code thread safe.
	/* These are volatile to prevent reads and writes to them from being optimized. */
	std::atomic<unsigned> read_pos, write_pos;

public:
	CircBuf() {
		buf = nullptr;
		clear();
	}

	~CircBuf() {
		delete[] buf;
	}

	void swap(CircBuf &rhs) {
		std::swap(size, rhs.size);
		std::swap(m_iBlockSize, rhs.m_iBlockSize);
		// std::swap does not support std::atomic<T*> as of C++14, therefore we must write our own
		// as swap() is only used in assignment operator overloading, efficiency is not critical and we may use the default memory order, sequential.
		// std::swap( read_pos, rhs.read_pos );
		// std::swap( write_pos, rhs.write_pos );
		const int rpos = read_pos.load();
		const int wpos = write_pos.load();
		read_pos.store(rhs.read_pos.load());
		write_pos.store(rhs.write_pos.load());
		rhs.read_pos.store(rpos);
		rhs.write_pos.store(wpos);
		std::swap(buf, rhs.buf);
	}

	CircBuf &operator=(const CircBuf &rhs) {
		CircBuf c(rhs);
		this->swap(c);
		return *this;
	}

	CircBuf(const CircBuf &cpy) {
		size = cpy.size;
		// same rationale as above, efficiency is not critical, we may use the default memory ordering
		// read_pos = cpy.read_pos;
		// write_pos = cpy.write_pos;
		read_pos.store(cpy.read_pos.load());
		write_pos.store(cpy.write_pos.load());
		m_iBlockSize = cpy.m_iBlockSize;
		// if there are elements in array, copy memory over to new array
		if (size) {
			buf = new T[size];
			std::memcpy(buf, cpy.buf, size * sizeof(T));
		} else {
			buf = nullptr;
		}
	}

	/* Return the number of elements available to read. */
	unsigned num_readable() const {
		// efficiency is not critical, as observed in this project
		// default memory ordering is sufficient for both as we are simply requsting a snapshot of the rpos & wpos.
		// Does not affect rpos/wpos updates
		// const int rpos = read_pos;
		// const int wpos = write_pos;
		const int rpos = read_pos.load();
		const int wpos = write_pos.load();

		if (rpos < wpos)
			/* The buffer looks like "eeeeDDDDeeee" (e = empty, D = data). */
			return wpos - rpos;
		else if (rpos > wpos)
			/* The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data). */
			return size - (rpos - wpos);
		else
			// if( rpos == wpos )
			/* The buffer looks like "eeeeeeeeeeee" (e = empty, D = data). */
			return 0;
	}

	/* Return the number of writable elements. */
	unsigned num_writable() const {
		// same rationale as num_readable() above
		// const int rpos = read_pos;
		// const int wpos = write_pos;
		const int rpos = read_pos.load();
		const int wpos = write_pos.load();

		int ret;
		if (rpos < wpos)
			/* The buffer looks like "eeeeDDDDeeee" (e = empty, D = data). */
			ret = size - (wpos - rpos);
		else if (rpos > wpos)
			/* The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data). */
			ret = rpos - wpos;
		else
			// if( rpos == wpos )
			/* The buffer looks like "eeeeeeeeeeee" (e = empty, D = data). */
			ret = size;

		/* Subtract the blocksize, to account for the element that we never fill
		 * while keeping the entries aligned to m_iBlockSize. */
		// m_iBlockSize is the reserved #bytes
		return ret - m_iBlockSize;
	}

	unsigned capacity() const {
		return size;
	}

	void reserve(unsigned n, int iBlockSize = 1) {
		// What if iBlockSize < 1?
		m_iBlockSize = iBlockSize;
		clear();
		delete[] buf;
		buf = nullptr;

		/* Reserve an extra byte.  We'll never fill more than n bytes; the extra
		 * byte is to guarantee that read_pos != write_pos when the buffer is full,
		 * since that would be ambiguous with an empty buffer. */
		if (n != 0) {
			size = n + 1;// +1 ensures that if size%iBlockSize = 0, we set size = size+iBlockSize,
						 // otherwise does nothing, and size proceeds to be rounded up
						 // this ensure we reserved at least 1 byte to the circular buffer
			size = ((size + iBlockSize - 1) / iBlockSize) * iBlockSize; // round up to the nearest mod(iBlockSize)

			buf = new T[size];
		} else
			size = 0;
	}

	void clear() {
		// only called in constructor and self.reserve(), which is only used in constructor of socketDrainClass obj.
		// Therefore relaxed memory order is sufficient
		write_pos.store(0, std::memory_order_relaxed);
		read_pos.store(0, std::memory_order_relaxed);
	}

	/* Indicate that n elements have been written. */
	// wrap around buffer array when reach end
	void advance_write_pointer(int n) {
		// write_pos = (write_pos + n) % size;
		write_pos.store((write_pos.load(std::memory_order_relaxed) + n) % size,
				std::memory_order_release);
	}

	/* Indicate that n elements have been read. */
	// wrap around buffer array when reach end
	void advance_read_pointer(int n) {
		// read_pos = (read_pos + n) % size;
		read_pos.store((read_pos.load(std::memory_order_relaxed) + n) % size,
				std::memory_order_release);
	}

	// pPointers[0] = HEAD of the empty buffer array chunk AFTER data chunk (wpos address)
	// pPointers[1] = HEAD of the empty buffer array chunk BEFORE data chunk
	//				set to buf when HEAD of buf is empty and the action of writing can possibly wrap around buf array
	// pSize[0] = size of empty buffer AFTER wpos in buffer array (subtracting reserved space)
	// pSize[1] = size of empty buffer BEFORE wpos in buffer array (subtracting reserved space)
	void get_write_pointers(T *pPointers[2], unsigned pSizes[2]) {
		// const int rpos = read_pos;
		// const int wpos = write_pos;
		const int wpos = write_pos.load(std::memory_order_relaxed);
		const int rpos = read_pos.load(std::memory_order_acquire);

		if (rpos <= wpos) {
			/* The buffer looks like "eeeeDDDDeeee" or "eeeeeeeeeeee" (e = empty, D = data). */
			pPointers[0] = buf + wpos;	// return
			pPointers[1] = buf;

			pSizes[0] = size - wpos;
			pSizes[1] = rpos;
		} else if (rpos > wpos) {
			/* The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data). */
			pPointers[0] = buf + wpos;
			pPointers[1] = nullptr;

			pSizes[0] = rpos - wpos;
			pSizes[1] = 0;
		}

		/* Subtract the blocksize, to account for the element that we never fill
		 * while keeping the entries aligned to m_iBlockSize. */
		if (pSizes[1])
			pSizes[1] -= m_iBlockSize;
		else
			pSizes[0] -= m_iBlockSize;
	}

	/* Like get_write_pointers, but only return the first range available. */
	// i.e., return: HEAD of the empty buffer array chunk AFTER data chunk (wpos address)
	// fill pSizes with: size of empty buffer AFTER wpos in buffer array (subtracting reserved space)
	T *get_write_pointer(unsigned *pSizes) {
		T *pBothPointers[2];
		unsigned iBothSizes[2];
		get_write_pointers(pBothPointers, iBothSizes);
		*pSizes = iBothSizes[0];
		return pBothPointers[0];
	}

	// pPointers[0] = HEAD of the filled buffer array chunk AFTER empty chunk (rpos address)
	// pPointers[1] = HEAD of the filled buffer array chunk BEFORE empty chunk
	//				set to buf when data chunk wrapping around buf array happens
	// pSize[0] = size of empty buffer AFTER rpos in buffer array
	// pSize[1] = size of empty buffer BEFORE rpos in buffer array
	void get_read_pointers(T *pPointers[2], unsigned pSizes[2]) {
		// const int rpos = read_pos;
		// const int wpos = write_pos;
		const int rpos = read_pos.load(std::memory_order_relaxed);
		const int wpos = write_pos.load(std::memory_order_acquire);

		if (rpos < wpos) {
			/* The buffer looks like "eeeeDDDDeeee" (e = empty, D = data). */
			pPointers[0] = buf + rpos;
			pPointers[1] = nullptr;

			pSizes[0] = wpos - rpos;
			pSizes[1] = 0;
		} else if (rpos > wpos) {
			/* The buffer looks like "DDeeeeeeeeDD" (e = empty, D = data). */
			pPointers[0] = buf + rpos;
			pPointers[1] = buf;

			pSizes[0] = size - rpos;
			pSizes[1] = wpos;
		} else {
			/* The buffer looks like "eeeeeeeeeeee" (e = empty, D = data). */
			pPointers[0] = nullptr;
			pPointers[1] = nullptr;

			pSizes[0] = 0;
			pSizes[1] = 0;
		}
	}

	/* Write buffer_size elements from buffer into the circular buffer object,
	 * and advance the write pointer.  Return the number of elements that were
	 * able to be written.  If
	 * the data will not fit entirely, as much data as possible will be fit
	 * in. */
	unsigned write(const T *buffer, unsigned buffer_size) {
		using std::min;
		using std::max;
		T *p[2];
		unsigned sizes[2];
		get_write_pointers(p, sizes);

		unsigned max_write_size = sizes[0] + sizes[1];
		if (buffer_size > max_write_size)
			buffer_size = max_write_size;

		const int from_first = min(buffer_size, sizes[0]);
		// cpy as much data as possible from wpos to (endata than the first empty chunk
		// continue writing to the second dchunk
		std::memcpy(p[0], buffer, from_first * sizeof(T));
		if (buffer_size > sizes[0])
			std::memcpy(p[1], buffer + from_first,
					max(buffer_size - sizes[0], 0u) * sizeof(T));
		// max(buffer_size-sizes[0], 0u) ensures buffer_size < sizes[0], otherwise use memcpy.length = 0
		// QUESTION: this doesn't seem to fulfill the claim that:
		// "as much data as possible will be fit"

		// update wpos
		advance_write_pointer(buffer_size);

		return buffer_size;
	}

	/* Read buffer_size elements into buffer from the circular buffer object,
	 * and advance the read pointer.  Return the number of elements that were
	 * read.  If buffer_size elements cannot be read, as many elements as
	 * possible will be read */
	unsigned read(T *buffer, unsigned buffer_size) {
		using std::max;
		using std::min;
		T *p[2];
		unsigned sizes[2];
		get_read_pointers(p, sizes);

		unsigned max_read_size = sizes[0] + sizes[1];
		if (buffer_size > max_read_size)
			buffer_size = max_read_size;

		const int from_first = min(buffer_size, sizes[0]);
		std::memcpy(buffer, p[0], from_first * sizeof(T));
		if (buffer_size > sizes[0])
			std::memcpy(buffer + from_first, p[1],
					max(buffer_size - sizes[0], 0u) * sizeof(T));

		/* Set the data that we just read to 0xFF.  This way, if we're passing pointers
		 * through, we can tell if we accidentally get a stale pointer. */
		// QUESTION: what does craig mean? What's the objective of resetting these
		std::memset(p[0], 0xFF, from_first * sizeof(T));
		if (buffer_size > sizes[0])
			std::memset(p[1], 0xFF,
					max(buffer_size - sizes[0], 0u) * sizeof(T));

		// update rpos
		advance_read_pointer(buffer_size);
		return buffer_size;
	}
};

#endif

/*
 * Copyright (c) 2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

