/*

Copyright (c) 2007 Manvel Avetisian. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided	with the distribution.
    * Neither the name of Manvel Avetisian nor the names of his contributors may
	be used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#if !defined(__DISKHASH__VBE_H__)
#define __DISKHASH__VBE_H__

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

#endif //!defined(__DISKHASH__VBE_H__)
