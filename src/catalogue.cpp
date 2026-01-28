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
