#pragma once

#include <limits.h>
#include <string>
#include <stdexcept>

#include "settings.h"
#include "file_map.h"

namespace diskhash {

class catalogue {
public:
	typedef size_t value_type;
	typedef value_type *iterator;
	typedef const value_type *const_iterator;

	static const size_t HASH_BITS = sizeof(hash_t) * CHAR_BIT;
	static const size_t INVALID_BLOCK_ID = size_t(-1);

	catalogue(const char *filename, size_t prefix_bits, bool read_only = false);

	value_type &find(hash_t const &hash)
	{
		return layout_->buffer[size_t((hash & layout_->prefix_mask) >> layout_->prefix_shift)];
	}

	value_type const &find(hash_t const &hash) const
	{
		return layout_->buffer[size_t((hash & layout_->prefix_mask) >> layout_->prefix_shift)];
	}

	void set(hash_t const &hash, size_t offset, value_type value);
	void split();

	iterator begin() {
		return &layout_->buffer[0];
	}

	const_iterator begin() const {
		return &layout_->buffer[0];
	}

	iterator end() {
		return begin() + layout_->buffer_size;
	}

	const_iterator end() const {
		return begin() + layout_->buffer_size;
	}

	size_t prefix_bits() const {
		return layout_->prefix_bits;
	}

	size_t prefix_shift() const {
		return layout_->prefix_shift;
	}

	size_t bytes_allocated() const {
		return file_map_.length();
	}

	void close() {
		file_map_.close();
		layout_ = 0;
	}

private:
	static const unsigned SIGNATURE = 0x99fa7e8e;

#pragma pack(push, 1)
	struct layout_t {
		unsigned signature;
		size_t prefix_bits, prefix_shift;
		hash_t prefix_mask;
		size_t buffer_size;
		value_type buffer[1];
	};
#pragma pack(pop)

	file_map file_map_;
	layout_t *layout_;
};

// namespace diskhash
}
