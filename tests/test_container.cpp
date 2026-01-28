
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <map>
#include <iostream>
#include <string_view>

#include "wrapped_hash_map.h"
#include "container.h"

using namespace diskhash;

void test_container()
{
	typedef container<> container_type;
	typedef std::map<unsigned, unsigned> map_type;

	std::cout << "creating container...\n";

	container_type container("test_map");

	map_type map;

	std::cout << "creating bucket in container...\n";
	size_t bucket_id = container.create_bucket(0);

	std::cout << "filling bucket...\n";
	for(size_t i = 0; i < 100; i++)
	{
		unsigned key = unsigned(rand()) * unsigned(rand());
		unsigned value = unsigned(rand()) * unsigned(rand());

		if(map.find(key) != map.end())
		{
			continue;
		}

		container.get(bucket_id, ~key, wrap(key), wrap(value));

		map.insert(std::make_pair(key, value));
	}

	std::cout << "checking correctness of find()...\n";
	for(map_type::const_iterator it = map.begin(); it != map.end(); it++)
	{
		assert(*(unsigned *) container.find(bucket_id, ~it->first, wrap(it->first)) == it->second);
	}

	std::cout << "splitting bucket " << bucket_id << "...\n";
	size_t new_bucket_id = container.split(bucket_id);

	std::cout << "checking correctness of find()...\n";
	for(map_type::const_iterator it = map.begin(); it != map.end(); it++)
	{
		unsigned hash = ~it->first;

		if(hash & (1u << (sizeof(unsigned) * CHAR_BIT - 1)))
		{
			assert(*(unsigned *) container.find(new_bucket_id, hash, wrap(it->first)) == it->second);
		}
		else
		{
			assert(*(unsigned *) container.find(bucket_id, hash, wrap(it->first)) == it->second);
		}
	}

	container.close();
	unlink("test_map");
}

void test_container_remove()
{
	typedef container<> container_type;
	typedef std::map<unsigned, unsigned> map_type;

	std::cout << "creating container for remove test...\n";

	container_type container("test_map_rm");

	map_type map;

	size_t bucket_id = container.create_bucket(0);

	std::cout << "filling bucket...\n";
	for(size_t i = 0; i < 100; i++)
	{
		unsigned key = unsigned(rand()) * unsigned(rand());
		unsigned value = unsigned(rand()) * unsigned(rand());

		if(map.find(key) != map.end())
		{
			continue;
		}

		container.get(bucket_id, ~key, wrap(key), wrap(value));
		map.insert(std::make_pair(key, value));
	}

	std::cout << "checking remove_record returns false for non-existent key...\n";
	{
		unsigned missing_key = 0xdeadbeef;
		assert(!container.remove_record(bucket_id, ~missing_key, wrap(missing_key)));
	}

	std::cout << "removing records one by one and verifying...\n";
	size_t removed = 0;
	for(map_type::iterator it = map.begin(); it != map.end(); )
	{
		unsigned key = it->first;
		unsigned hash = ~key;

		// remove from container
		assert(container.remove_record(bucket_id, hash, wrap(key)));
		removed++;

		// removing again should return false
		assert(!container.remove_record(bucket_id, hash, wrap(key)));

		it = map.erase(it);

		// remaining records should still be findable
		for(map_type::const_iterator it2 = map.begin(); it2 != map.end(); it2++)
		{
			assert(*(unsigned *) container.find(bucket_id, ~it2->first, wrap(it2->first)) == it2->second);
		}
	}

	std::cout << "removed " << removed << " records successfully\n";

	container.close();
	unlink("test_map_rm");
}
