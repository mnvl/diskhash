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


#if !defined(__DISKHASH__WRAPPED_HASH_MAP_H__)
#define __DISKHASH__WRAPPED_HASH_MAP_H__

#include "hash_map.h"

namespace diskhash {

template<class Key, class Value, class HashFunc, size_t BucketSize = DEFAULT_BUCKET_SIZE>
class wrapped_hash_map {
public:
	typedef Key key_type;
	typedef Value value_type;
	typedef HashFunc hash_function_type;

	wrapped_hash_map(const char *filename, bool read_only = false, hash_function_type hash_func = hash_function_type()):
		hash_map_(filename, read_only),
		hash_function_(hash_func)
	{
	}

	value_type &operator[](const key_type &key)
	{
		hash_t hash = hash_function_(key);
		return *(value_type *) hash_map_.get(hash, wrap(key), wrap(value_type()));
	}

	size_t bytes_allocated() const
	{
		return hash_map_.bytes_allocated();
	}

	void close() {
		hash_map_.close();
	}

private:
	hash_function_type hash_function_;
	hash_map<BucketSize> hash_map_;
};

// namespace diskhash
}

#endif //!defined(__DISKHASH__WRAPPED_HASH_MAP_H__)
