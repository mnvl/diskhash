#pragma once

#include <type_traits>
#include "hash_map.h"

namespace diskhash {


static std::string_view wrap(std::string const &str)
{
	return std::string_view(str.data(), str.size());
}

template<class T>
static std::string_view wrap(T const &t)
{
	return std::string_view(reinterpret_cast<const char *>(&t), sizeof(T));
}


template<class Key, class Value, class HashFunc, size_t BucketSize = DEFAULT_BUCKET_SIZE>
class wrapped_hash_map {
	static_assert(std::is_trivial_v<Value>, "Value must be a trivial type");
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
		auto result = hash_map_.get(hash, wrap(key), wrap(value_type()));
		return *(value_type *)(const_cast<char*>(result->data()));
	}

	bool remove(const key_type &key)
	{
		hash_t hash = hash_function_(key);
		return hash_map_.remove(hash, wrap(key));
	}

	size_t bytes_allocated() const
	{
		return hash_map_.bytes_allocated();
	}

	void close() {
		hash_map_.close();
	}

	typename hash_map<BucketSize>::const_iterator begin() const { return hash_map_.begin(); }
	typename hash_map<BucketSize>::const_iterator end() const { return hash_map_.end(); }

private:
	hash_function_type hash_function_;
	hash_map<BucketSize> hash_map_;
};

// namespace diskhash
}
