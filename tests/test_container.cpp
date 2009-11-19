
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <map>
#include <iostream>

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

		container.get(bucket_id, ~key, diskhash::wrap(key), diskhash::wrap(value));

		map.insert(std::make_pair(key, value));
	}

	std::cout << "checking correctness of find()...\n";
	for(map_type::const_iterator it = map.begin(); it != map.end(); it++)
	{
		assert(*(unsigned *) container.find(bucket_id, ~it->first, diskhash::wrap(it->first)) == it->second);
	}

	std::cout << "splitting bucket " << bucket_id << "...\n";
	size_t new_bucket_id = container.split(bucket_id);

	std::cout << "checking correctness of find()...\n";
	for(map_type::const_iterator it = map.begin(); it != map.end(); it++)
	{
		unsigned hash = ~it->first;

		if(hash & (1u << (sizeof(unsigned) * CHAR_BIT - 1)))
		{
			assert(*(unsigned *) container.find(new_bucket_id, hash, diskhash::wrap(it->first)) == it->second);
		}
		else
		{
			assert(*(unsigned *) container.find(bucket_id, hash, diskhash::wrap(it->first)) == it->second);
		}
	}

	container.close();
	unlink("test_map");
}
