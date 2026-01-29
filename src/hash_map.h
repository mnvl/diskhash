#pragma once

#include <optional>
#include <string_view>
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

	std::optional<std::string_view> get(hash_t hash, std::string_view key, std::string_view default_value)
	{
		size_t bucket_id = catalogue_.find(hash);

		if(auto result = container_.find_record(bucket_id, hash, key))
		{
			return result;
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

	class const_iterator {
	public:
		using value_type = std::pair<std::string_view, std::string_view>;

		const_iterator(): map_(nullptr), catalogue_index_(0), bucket_id_(0), byte_offset_(0) {}

		const_iterator(const hash_map *map, size_t cat_index):
			map_(map), catalogue_index_(cat_index), bucket_id_(0), byte_offset_(0)
		{
			if(catalogue_index_ < buffer_size())
			{
				bucket_id_ = buffer()[catalogue_index_];
				find_next_record();
			}
			else
			{
				map_ = nullptr;
			}
		}

		value_type operator*() const
		{
			record_view rv;
			size_t offset = byte_offset_;
			map_->container_.read_record(bucket_id_, offset, rv);
			return {rv.key, rv.value};
		}

		const_iterator &operator++()
		{
			// skip past the current record
			record_view rv;
			map_->container_.read_record(bucket_id_, byte_offset_, rv);
			// now find the next valid record
			find_next_record();
			return *this;
		}

		bool operator==(const const_iterator &o) const
		{
			if(!map_ && !o.map_) return true;
			if(!map_ || !o.map_) return false;
			return catalogue_index_ == o.catalogue_index_
				&& bucket_id_ == o.bucket_id_
				&& byte_offset_ == o.byte_offset_;
		}

		bool operator!=(const const_iterator &o) const { return !(*this == o); }

	private:
		const hash_map *map_;
		size_t catalogue_index_;
		size_t bucket_id_;
		size_t byte_offset_;

		const catalogue::value_type *buffer() const { return map_->catalogue_.begin(); }
		size_t buffer_size() const { return map_->catalogue_.end() - map_->catalogue_.begin(); }

		// try to find a record at the current (bucket_id_, byte_offset_) or later.
		// if no record is found in the current bucket chain / catalogue entries,
		// advances to the next unique catalogue entry. Sets map_=nullptr at end.
		void find_next_record()
		{
			record_view rv;

			for(;;)
			{
				// check if there's a record at the current position
				size_t probe = byte_offset_;
				if(map_->container_.read_record(bucket_id_, probe, rv))
					return; // byte_offset_ still points at the start of this record

				// no more records in this bucket, try overflow chain
				size_t next = map_->container_.next_bucket(bucket_id_);
				if(next != container_type::invalid_bucket_id())
				{
					bucket_id_ = next;
					byte_offset_ = 0;
					continue;
				}

				// move to next unique catalogue entry
				++catalogue_index_;
				while(catalogue_index_ < buffer_size()
					&& buffer()[catalogue_index_] == buffer()[catalogue_index_ - 1])
				{
					++catalogue_index_;
				}

				if(catalogue_index_ >= buffer_size())
				{
					map_ = nullptr;
					return;
				}

				bucket_id_ = buffer()[catalogue_index_];
				byte_offset_ = 0;
			}
		}
	};

	const_iterator begin() const
	{
		return const_iterator(this, 0);
	}

	const_iterator end() const
	{
		return const_iterator();
	}

private:
	typedef container<BucketSize> container_type;

	catalogue catalogue_;
	container_type container_;
};

// namespace diskhash
}
