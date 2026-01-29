#include <assert.h>
#include "container.h"
#include "vbe.h"

template<size_t BucketSize>
diskhash::container<BucketSize>::container(const char *filename, bool read_only):
	file_map_(filename, read_only, sizeof(layout_t))
{
	layout_ = (layout_t *) file_map_.start();

	if(layout_->signature == 0)
	{
		layout_->signature = SIGNATURE;
		layout_->first_free_bucket_id = INVALID_BUCKET_ID;
	}
	else if(layout_->signature != SIGNATURE)
	{
		throw std::runtime_error(std::string("invalid hash container signature in file ") + filename);
	}
}

template<size_t BucketSize>
size_t diskhash::container<BucketSize>::create_bucket(size_t prefix_bits)
{
	size_t bucket_id;

	if(layout_->first_free_bucket_id == INVALID_BUCKET_ID)
	{
		size_t bytes_needed = layout_->buckets_count * sizeof(bucket_t) + sizeof(layout_t);

		if(bytes_needed > file_map_.length())
		{
			file_map_.resize((bytes_needed * 11) / 10);
			layout_ = (layout_t *) file_map_.start();
		}

		bucket_id = layout_->buckets_count++;
	}
	else
	{
		bucket_id = layout_->first_free_bucket_id;
		layout_->first_free_bucket_id = layout_->buckets[bucket_id].next_bucket_id;
	}

	bucket_t *bucket_ptr = &layout_->buckets[bucket_id];
	bucket_ptr->prefix_bits = prefix_bits;
	bucket_ptr->bytes_used = 0;
	bucket_ptr->next_bucket_id = INVALID_BUCKET_ID;

	return bucket_id;
}

template<size_t BucketSize>
std::string_view diskhash::container<BucketSize>::create_record(size_t bucket_id, hash_t const &hash, std::string_view key,
	std::string_view value)
{
	bucket_t *bucket_ptr = &layout_->buckets[bucket_id];

	size_t bytes_required = sizeof(hash_t) + vbe::length(key.size()) + key.size()
		+ vbe::length(value.size()) + value.size();

	while(bucket_ptr->bytes_used + bytes_required > BUCKET_SIZE)
	{
		if(bucket_ptr->next_bucket_id == INVALID_BUCKET_ID)
		{
			size_t new_bucket_id = create_bucket(bucket_ptr->prefix_bits);

			layout_->buckets[bucket_id].next_bucket_id = new_bucket_id;

			bucket_id = new_bucket_id;
			bucket_ptr = &layout_->buckets[bucket_id];
		}
		else
		{
			bucket_id = bucket_ptr->next_bucket_id;
			bucket_ptr = &layout_->buckets[bucket_ptr->next_bucket_id];
		}
	}

	const unsigned char *key_bytes = reinterpret_cast<const unsigned char *>(key.data());
	const unsigned char *value_bytes = reinterpret_cast<const unsigned char *>(value.data());

	unsigned char *cursor = bucket_ptr->data + bucket_ptr->bytes_used;
	cursor = std::copy((unsigned char *) &hash, (unsigned char *) (&hash + 1), cursor);
	cursor = vbe::write(cursor, key.size());
	cursor = vbe::write(cursor, value.size());
	cursor = std::copy(key_bytes, key_bytes + key.size(), cursor);
	std::copy(value_bytes, value_bytes + value.size(), cursor);

	bucket_ptr->bytes_used += bytes_required;

	return std::string_view(reinterpret_cast<const char *>(cursor), value.size());
}

template<size_t BucketSize>
std::optional<std::string_view> diskhash::container<BucketSize>::find_value(size_t bucket_id, const hash_t &hash, std::string_view key) const
{
	const unsigned char *key_bytes = reinterpret_cast<const unsigned char *>(key.data());

	while(bucket_id != INVALID_BUCKET_ID)
	{
		bucket_t *bucket_ptr = &layout_->buckets[bucket_id];

		unsigned char *cursor = bucket_ptr->data;

		while(cursor != bucket_ptr->data + bucket_ptr->bytes_used)
		{
			hash_t record_hash;
			size_t key_length, value_length;

			std::copy(cursor, cursor + sizeof(hash_t), (unsigned char *) &record_hash);
			cursor = vbe::read(cursor + sizeof(hash_t), key_length);
			cursor = vbe::read(cursor, value_length);

			if(record_hash == hash && key.size() == key_length && std::equal(cursor, cursor + key_length, key_bytes))
			{
				return std::string_view(reinterpret_cast<const char *>(cursor + key_length), value_length);
			}

			cursor += key_length;
			cursor += value_length;

			assert(cursor <= bucket_ptr->data + bucket_ptr->bytes_used);
		}

		bucket_id = bucket_ptr->next_bucket_id;
	}

	return std::nullopt;
}

template<size_t BucketSize>
std::optional<std::string_view> diskhash::container<BucketSize>::find_record(size_t bucket_id, const hash_t &hash, std::string_view key) const
{
	return find_value(bucket_id, hash, key);
}

template<size_t BucketSize>
bool diskhash::container<BucketSize>::remove_record(size_t bucket_id, const hash_t &hash, std::string_view key)
{
	const unsigned char *key_bytes = reinterpret_cast<const unsigned char *>(key.data());

	while(bucket_id != INVALID_BUCKET_ID)
	{
		bucket_t *bucket_ptr = &layout_->buckets[bucket_id];

		unsigned char *cursor = bucket_ptr->data;

		while(cursor != bucket_ptr->data + bucket_ptr->bytes_used)
		{
			unsigned char *record_start = cursor;

			hash_t record_hash;
			size_t key_length, value_length;

			std::copy(cursor, cursor + sizeof(hash_t), (unsigned char *) &record_hash);
			cursor = vbe::read(cursor + sizeof(hash_t), key_length);
			cursor = vbe::read(cursor, value_length);

			size_t record_length = (cursor - record_start) + key_length + value_length;

			if(record_hash == hash && key.size() == key_length && std::equal(cursor, cursor + key_length, key_bytes))
			{
				unsigned char *record_end = record_start + record_length;
				unsigned char *bucket_end = bucket_ptr->data + bucket_ptr->bytes_used;
				std::copy(record_end, bucket_end, record_start);
				bucket_ptr->bytes_used -= record_length;
				return true;
			}

			cursor += key_length;
			cursor += value_length;

			assert(cursor <= bucket_ptr->data + bucket_ptr->bytes_used);
		}

		bucket_id = bucket_ptr->next_bucket_id;
	}

	return false;
}

template<size_t BucketSize>
size_t diskhash::container<BucketSize>::split(size_t bucket_id)
{
	size_t prefix_bits = ++layout_->buckets[bucket_id].prefix_bits;

	size_t bit0_bucket_id = bucket_id;
	size_t bit1_bucket_id = create_bucket(prefix_bits);

	size_t result_bucket_id = bit1_bucket_id;

	hash_t new_bit = hash_t(1) << (HASH_BITS - prefix_bits);

	bucket_t *bit0_bucket_ptr = &layout_->buckets[bit0_bucket_id];
	bucket_t *bit1_bucket_ptr = &layout_->buckets[bit1_bucket_id];

	unsigned char *bit0_bucket_put = bit0_bucket_ptr->data;
	unsigned char *bit1_bucket_put = bit1_bucket_ptr->data;

	for(size_t get_bucket_id = bucket_id; get_bucket_id != INVALID_BUCKET_ID;
		get_bucket_id = layout_->buckets[get_bucket_id].next_bucket_id)
	{
		bucket_t *get_bucket_ptr = &layout_->buckets[get_bucket_id];
		unsigned char *get_ptr = get_bucket_ptr->data;

		size_t get_last = get_bucket_ptr->bytes_used;
		get_bucket_ptr->bytes_used = 0;

		while(get_ptr != get_bucket_ptr->data + get_last)
		{
			hash_t hash;
			size_t key_length, value_length;

			std::copy(get_ptr, get_ptr + sizeof(hash_t), (unsigned char *) &hash);
			unsigned char *cursor = vbe::read(get_ptr + sizeof(hash_t), key_length);
			cursor = vbe::read(cursor, value_length);

			size_t record_length = (cursor - get_ptr) + key_length + value_length;
			assert(record_length == sizeof(hash_t) + vbe::length(key_length) + key_length
				+ vbe::length(value_length) + value_length);

			if(hash & new_bit)
			{
				if(bit1_bucket_ptr->bytes_used + record_length > BUCKET_SIZE)
				{
					assert(bit1_bucket_put - bit1_bucket_ptr->data == (ptrdiff_t) bit1_bucket_ptr->bytes_used);

					size_t offset = get_ptr - get_bucket_ptr->data;

					size_t new_bucket_id = create_bucket(prefix_bits);

					layout_->buckets[bit1_bucket_id].next_bucket_id = new_bucket_id;
					bit1_bucket_id = new_bucket_id;

					bit0_bucket_ptr = &layout_->buckets[bit0_bucket_id];
					bit1_bucket_ptr = &layout_->buckets[bit1_bucket_id];

					bit0_bucket_put = bit0_bucket_ptr->data + bit0_bucket_ptr->bytes_used;
					bit1_bucket_put = bit1_bucket_ptr->data;

					get_bucket_ptr = &layout_->buckets[get_bucket_id];
					get_ptr = get_bucket_ptr->data + offset;
				}

				bit1_bucket_put = std::copy(get_ptr, get_ptr + record_length, bit1_bucket_put);

				get_ptr += record_length;
				bit1_bucket_ptr->bytes_used += record_length;
			}
			else
			{
				if(bit0_bucket_ptr->bytes_used + record_length > BUCKET_SIZE)
				{
					assert(bit0_bucket_put - bit0_bucket_ptr->data == (ptrdiff_t) bit0_bucket_ptr->bytes_used);

					if(bit0_bucket_ptr->next_bucket_id != INVALID_BUCKET_ID)
					{
						bit0_bucket_id = bit0_bucket_ptr->next_bucket_id;
						bit0_bucket_ptr = &layout_->buckets[bit0_bucket_id];
					}
					else
					{
						size_t offset = get_ptr - get_bucket_ptr->data;

						size_t new_bucket_id = create_bucket(prefix_bits);
						layout_->buckets[bit0_bucket_id].next_bucket_id = new_bucket_id;

						bit0_bucket_id = new_bucket_id;
						bit0_bucket_ptr = &layout_->buckets[bit0_bucket_id];

						get_bucket_ptr = &layout_->buckets[get_bucket_id];
						get_ptr = get_bucket_ptr->data + offset;

						bit1_bucket_ptr = &layout_->buckets[bit1_bucket_id];
						bit1_bucket_put = bit1_bucket_ptr->data + bit1_bucket_ptr->bytes_used;
					}

					bit0_bucket_ptr->prefix_bits = prefix_bits;
					bit0_bucket_put = bit0_bucket_ptr->data;
				}

				bit0_bucket_put = std::copy(get_ptr, get_ptr + record_length, bit0_bucket_put);

				get_ptr += record_length;
				bit0_bucket_ptr->bytes_used += record_length;
			}
		}
	}

	size_t free_bucket_id = bit0_bucket_ptr->next_bucket_id;
	bit0_bucket_ptr->next_bucket_id = INVALID_BUCKET_ID;

	while(free_bucket_id != INVALID_BUCKET_ID)
	{
		bit0_bucket_ptr = &layout_->buckets[free_bucket_id];

		assert(bit0_bucket_ptr->bytes_used == 0);

		size_t next_bucket_id = bit0_bucket_ptr->next_bucket_id;

		bit0_bucket_ptr->next_bucket_id = layout_->first_free_bucket_id;
		layout_->first_free_bucket_id = free_bucket_id;

		free_bucket_id = next_bucket_id;
	}

	return result_bucket_id;
}

template<size_t BucketSize>
bool diskhash::container<BucketSize>::read_record(size_t bucket_id, size_t &byte_offset, record_view &rv) const
{
	bucket_t *bucket_ptr = &layout_->buckets[bucket_id];

	if(byte_offset >= bucket_ptr->bytes_used)
		return false;

	unsigned char *cursor = bucket_ptr->data + byte_offset;

	hash_t record_hash;
	size_t key_length, value_length;

	std::copy(cursor, cursor + sizeof(hash_t), (unsigned char *) &record_hash);
	cursor = vbe::read(cursor + sizeof(hash_t), key_length);
	cursor = vbe::read(cursor, value_length);

	rv.hash = record_hash;
	rv.key = std::string_view(reinterpret_cast<const char *>(cursor), key_length);
	rv.value = std::string_view(reinterpret_cast<const char *>(cursor + key_length), value_length);

	byte_offset = (cursor + key_length + value_length) - bucket_ptr->data;

	return true;
}

template class diskhash::container<>;
