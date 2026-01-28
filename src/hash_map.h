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

#if !defined(__DISKHASH__HASH_MAP_H__)
#define __DISKHASH__HASH_MAP_H__

#include <optional>
#include <string_view>
#include "record.h"
#include "container.h"
#include "catalogue.h"

namespace diskhash {

template<size_t BucketSize = DEFAULT_BUCKET_SIZE>
class hash_map {
public:
	hash_map(const char *filename, bool read_only = false):
		catalogue_((std::string(filename) + "cat").c_str(), 1, read_only),
		container_((std::string(filename) + "dat").c_str(), read_only)
	{
		if(container_.buckets_count() == 0)
		{
			catalogue_.find(hash_t(0)) = container_.create_bucket(1);
			catalogue_.find(~hash_t(0)) = container_.create_bucket(1);
		}
	}

	void *get(hash_t hash, std::string_view key, std::string_view default_value)
	{
		size_t bucket_id = catalogue_.find(hash);

		if(void *result_ptr = container_.find(bucket_id, hash, key))
		{
			return result_ptr;
		}

		if(container_.bucket_to_split(bucket_id))
		{
			if(container_.bucket_prefix_bits(bucket_id) == catalogue_.prefix_bits()
				&& (container_.buckets_count()) > (size_t(1) << catalogue_.prefix_bits()))
			{
				catalogue_.split();
			}

			if(container_.bucket_prefix_bits(bucket_id) < catalogue_.prefix_bits())
			{
				size_t new_bucket_id = container_.split(bucket_id);
				size_t prefix_bits = container_.bucket_prefix_bits(bucket_id);

				assert(prefix_bits == container_.bucket_prefix_bits(new_bucket_id));

				hash_t new_bit = (hash_t(1) << (HASH_BITS - prefix_bits));

				catalogue_.set(hash | new_bit, prefix_bits, new_bucket_id);

				if(hash & new_bit)
				{
					bucket_id = new_bucket_id;
				}

				assert(catalogue_.find(hash) == bucket_id);
			}
		}

		return container_.create_record(bucket_id, hash, key, default_value);
	}

	std::optional<std::string_view> find(hash_t hash, std::string_view key) const {
		size_t bucket_id = catalogue_.find(hash);
		return container_.find_record(bucket_id, hash, key);
	}

	bool remove(hash_t hash, std::string_view key) {
		size_t bucket_id = catalogue_.find(hash);
		return container_.remove_record(bucket_id, hash, key);
	}

	size_t bytes_allocated() const {
		return container_.bytes_allocated() + catalogue_.bytes_allocated();
	}

	void close() {
		catalogue_.close();
		container_.close();
	}

private:
	typedef container<BucketSize> container_type;

	catalogue catalogue_;
	container_type container_;
};

// namespace diskhash
}

#endif //!defined(__DISKHASH__HASH_MAP_H__)
