#pragma once

#include "settings.h"
#include <string>
#include <string_view>
#include <optional>
#include <stdexcept>
#include "file_map.h"

namespace diskhash {

struct record_view {
	hash_t hash;
	std::string_view key;
	std::string_view value;
};

template<size_t BucketSize = DEFAULT_BUCKET_SIZE>
class container {
public:
	static size_t const BUCKET_SIZE = BucketSize;

	container(const char *filename, bool read_only = false);

	// create bucket and return bucket id
	size_t create_bucket(size_t prefix_bits);

	// write record (hash, key, value) into bucket bucket_id, return stored value
	std::string_view create_record(size_t bucket_id, hash_t const &hash, std::string_view key,
		std::string_view value);

	// find record (hash, key, *) in bucket bucket_id and return value as string_view,
	// return nullopt if no such record found
	std::optional<std::string_view> find_record(size_t bucket_id, const hash_t &hash, std::string_view key) const;

	// parse record at byte_offset in bucket_id, fill rv, advance byte_offset.
	// returns false if byte_offset >= bytes_used (no more records in this bucket).
	bool read_record(size_t bucket_id, size_t &byte_offset, record_view &rv) const;

	// return the next_bucket_id for the given bucket, or INVALID_BUCKET_ID if none
	size_t next_bucket(size_t bucket_id) const
	{
		return layout_->buckets[bucket_id].next_bucket_id;
	}

	static size_t invalid_bucket_id() { return INVALID_BUCKET_ID; }

	// remove record (hash, key) from bucket chain starting at bucket_id
	// return true if found and removed, false if not found
	bool remove_record(size_t bucket_id, const hash_t &hash, std::string_view key);

	// find record (hash, key, *) in bucket bucket_id and return its value,
	// if no such record found -- create new record (key, value) in bucket bucket_id
	// and return the stored value
	std::string_view get(size_t bucket_id, const hash_t &hash, std::string_view key, std::string_view value)
	{
		if(auto result = find_record(bucket_id, hash, key))
		{
			return *result;
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
	// search bucket chain for record matching (hash, key) and return its value, or nullopt
	std::optional<std::string_view> find_value(size_t bucket_id, const hash_t &hash, std::string_view key) const;

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
