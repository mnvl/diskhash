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

#if !defined(container_h)
#define container_h

#include "settings.h"
#include <string>
#include <stdexcept>
#include "file_map.h"
#include "record.h"

namespace diskhash {

template<size_t BucketSize = DEFAULT_BUCKET_SIZE>
class container {
public:
	static size_t const BUCKET_SIZE = BucketSize;

	container(const char *filename, bool read_only = false);

	// create bucket and return bucket id
	size_t create_bucket(size_t prefix_bits);

	// write record (hash, key, *) into bucket bucket_id, return value is pointer to value field
	void *create_record(size_t bucket_id, hash_t const &hash, const_record const &key,
		const_record const &value);

	// find record (hash, key, *) in bucket bucket_id and return pointer to value,
	// return 0 if no such record found
	void *find(size_t bucket_id, const hash_t &hash, const_record const &key) const;

	// find record (hash, key, *) in bucket bucket_id and return value as record (data + length),
	// return record(0, 0) if no such record found
	record find_record(size_t bucket_id, const hash_t &hash, const_record const &key) const;

	// find record (hash, key, *) in bucket bucket_id and return pointer to value,
	// if no such record found -- create new one record (key, 0) in bucket bucket_id
	// and return pointer to value field, if no space left in bucket -- return 0
	void *get(size_t bucket_id, const hash_t &hash, const_record const &key, const_record const &value)
	{
		if(void *value = find(bucket_id, hash, key))
		{
			return value;
		}

		return create_record(bucket_id, hash, key, value);
	}

	// increase prefix_bits and move all records with hash & (1 << prefix_bits) into another bucket,
	// return id of that bucket
	size_t split(size_t bucket_id);

	size_t buckets_count() const {
		return layout_->buckets_count;
	}

	size_t bucket_bytes_used(size_t bucket_id) const
	{
		return layout_->buckets[bucket_id].bytes_used;
	}

	size_t bucket_prefix_bits(size_t bucket_id) const
	{
		return layout_->buckets[bucket_id].prefix_bits;
	}

	size_t bytes_allocated() const {
		return file_map_.length();
	}

	bool bucket_to_split(size_t bucket_id) const {
		bucket_t *first_bucket_ptr = &layout_->buckets[bucket_id];
		if(first_bucket_ptr->next_bucket_id == INVALID_BUCKET_ID) return false;

		bucket_t *second_bucket_ptr = &layout_->buckets[first_bucket_ptr->next_bucket_id];
		if(second_bucket_ptr->next_bucket_id != INVALID_BUCKET_ID) return true;

		return (first_bucket_ptr->bytes_used + second_bucket_ptr->bytes_used) > 3 * BUCKET_SIZE / 2;
	}

	void close() {
		file_map_.close();
		layout_ = 0;
	}

private:
	static const size_t INVALID_BUCKET_ID = size_t(-1);
	static const unsigned SIGNATURE = 0x69d3db7a;

#pragma pack(push, 1)
	struct bucket_t {
		size_t prefix_bits, bytes_used, next_bucket_id;
		unsigned char data[BUCKET_SIZE];
	};

	struct layout_t {
		unsigned signature;
		size_t buckets_count;
		size_t first_free_bucket_id;
		bucket_t buckets[1];
	};
#pragma pack(pop)

	file_map file_map_;
	layout_t *layout_;
};

// namespace diskhash
}

#endif //!defined(container_h)
