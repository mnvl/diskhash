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


#include "catalogue.h"

diskhash::catalogue::catalogue(const char *filename, size_t prefix_bits, bool read_only):
	file_map_(filename, read_only, sizeof(layout_t))
{
	layout_ = (layout_t *) file_map_.start();

	if(layout_->signature == 0)
	{
		file_map_.resize(sizeof(layout_t) + ((size_t(1) << prefix_bits) - 1) * sizeof(value_type));
		layout_ = (layout_t *) file_map_.start();

		layout_->signature = SIGNATURE;
		layout_->prefix_bits = prefix_bits;
		layout_->prefix_shift = HASH_BITS - layout_->prefix_bits;
		layout_->prefix_mask = ((hash_t(1) << layout_->prefix_bits) - 1) << layout_->prefix_shift;
		layout_->buffer_size = size_t(1) << layout_->prefix_bits;

		for(size_t i = 0; i < layout_->buffer_size; i++)
		{
			layout_->buffer[i] = INVALID_BLOCK_ID;
		}
	}
	else if(layout_->signature != SIGNATURE)
	{
		throw std::runtime_error(std::string("invalid hash catalogue signature in file ") + filename);
	}
}

void diskhash::catalogue::set(hash_t const &hash, size_t offset, value_type value)
{
	hash_t hash_copy = hash & ~((size_t(1) << (HASH_BITS - offset)) - 1);

	value_type *put = &layout_->buffer[size_t((hash_copy & layout_->prefix_mask) >> layout_->prefix_shift)];
	size_t count = size_t(1) << (layout_->prefix_bits - offset);

	while(count-- > 0)
	{
		*put++ = value;
	}
}

void diskhash::catalogue::split()
{
	size_t old_buffer_size = layout_->buffer_size;

	layout_->prefix_bits++;
	layout_->prefix_shift--;
	layout_->prefix_mask |= (hash_t(1) << layout_->prefix_shift);
	layout_->buffer_size = (old_buffer_size << 1);

	file_map_.resize(sizeof(layout_t) + (layout_->buffer_size - 1) * sizeof(value_type));
	layout_ = (layout_t *) file_map_.start();

	value_type *get = &layout_->buffer[old_buffer_size - 1];
	value_type *put = &layout_->buffer[layout_->buffer_size - 1];

	while(old_buffer_size-- != 0)
	{
		*put-- = *get;
		*put-- = *get--;
	}
}
