#pragma once

#include <limits.h>

namespace diskhash {
namespace vbe {

static size_t const MARK_OFFSET = CHAR_BIT - 1;
static unsigned char const MARK_MASK = ((unsigned char) 1) << MARK_OFFSET;
static unsigned char const DATA_MASK = (unsigned char) ~MARK_MASK;

template<class T>
unsigned char *write(unsigned char *ptr, T const &val)
{
	T val_copy = val;

	while(val_copy > DATA_MASK)
	{
		*ptr++ = (unsigned char) ((val_copy & DATA_MASK) | MARK_MASK);
		val_copy >>= MARK_OFFSET;
	}

	*ptr++ = (unsigned char) val_copy;

	return ptr;
}

template<class T>
unsigned char *read(unsigned char *ptr, T &val)
{
	size_t offset = 0;
	val = 0;

	while(*ptr & MARK_MASK)
	{
		val |= (*ptr++ & DATA_MASK) << offset;
		offset += MARK_OFFSET;
	}

	val |= *ptr++ << offset;

	return ptr;
}

// TODO: optimize, length(x) = (msb(x) / MARK_OFFSET) + 1
template<class T>
size_t length(T const &val)
{
	T val_copy = val;
	size_t result = 1;

	while(val_copy > DATA_MASK)
	{
		result++;
		val_copy >>= MARK_OFFSET;
	}

	return result;
}

// namespace vbe
}

// namespace diskhash
}
