
#include <boost/test/unit_test.hpp>
#include <limits.h>
#include <stdlib.h>
#include <map>
#include <string_view>

#include "wrapped_hash_map.h"
#include "container.h"

using namespace diskhash;

namespace {

void cleanup_files(const char *base)
{
	unlink(base);
}

struct basic_operations_fixture {
	basic_operations_fixture() { cleanup_files("test_map"); }
	~basic_operations_fixture() { cleanup_files("test_map"); }
};

struct remove_operations_fixture {
	remove_operations_fixture() { cleanup_files("test_map_rm"); }
	~remove_operations_fixture() { cleanup_files("test_map_rm"); }
};

}

BOOST_AUTO_TEST_SUITE(container_suite)

BOOST_FIXTURE_TEST_CASE(basic_operations, basic_operations_fixture)
{
	typedef container<> container_type;
	typedef std::map<unsigned, unsigned> map_type;

	container_type cont("test_map");

	map_type map;

	size_t bucket_id = cont.create_bucket(0);

	for(size_t i = 0; i < 100; i++)
	{
		unsigned key = unsigned(rand()) * unsigned(rand());
		unsigned value = unsigned(rand()) * unsigned(rand());

		if(map.find(key) != map.end())
		{
			continue;
		}

		cont.get(bucket_id, ~key, wrap(key), wrap(value));
		map.insert(std::make_pair(key, value));
	}

	for(map_type::const_iterator it = map.begin(); it != map.end(); it++)
	{
		BOOST_CHECK_EQUAL(*(unsigned *) cont.find(bucket_id, ~it->first, wrap(it->first)), it->second);
	}

	size_t new_bucket_id = cont.split(bucket_id);

	for(map_type::const_iterator it = map.begin(); it != map.end(); it++)
	{
		unsigned hash = ~it->first;

		if(hash & (1u << (sizeof(unsigned) * CHAR_BIT - 1)))
		{
			BOOST_CHECK_EQUAL(*(unsigned *) cont.find(new_bucket_id, hash, wrap(it->first)), it->second);
		}
		else
		{
			BOOST_CHECK_EQUAL(*(unsigned *) cont.find(bucket_id, hash, wrap(it->first)), it->second);
		}
	}

	cont.close();
}

BOOST_FIXTURE_TEST_CASE(remove_operations, remove_operations_fixture)
{
	typedef container<> container_type;
	typedef std::map<unsigned, unsigned> map_type;

	container_type cont("test_map_rm");

	map_type map;

	size_t bucket_id = cont.create_bucket(0);

	for(size_t i = 0; i < 100; i++)
	{
		unsigned key = unsigned(rand()) * unsigned(rand());
		unsigned value = unsigned(rand()) * unsigned(rand());

		if(map.find(key) != map.end())
		{
			continue;
		}

		cont.get(bucket_id, ~key, wrap(key), wrap(value));
		map.insert(std::make_pair(key, value));
	}

	unsigned missing_key = 0xdeadbeef;
	BOOST_CHECK(!cont.remove_record(bucket_id, ~missing_key, wrap(missing_key)));

	for(map_type::iterator it = map.begin(); it != map.end(); )
	{
		unsigned key = it->first;
		unsigned hash = ~key;

		BOOST_CHECK(cont.remove_record(bucket_id, hash, wrap(key)));
		BOOST_CHECK(!cont.remove_record(bucket_id, hash, wrap(key)));

		it = map.erase(it);

		for(map_type::const_iterator it2 = map.begin(); it2 != map.end(); it2++)
		{
			BOOST_CHECK_EQUAL(*(unsigned *) cont.find(bucket_id, ~it2->first, wrap(it2->first)), it2->second);
		}
	}

	cont.close();
}

BOOST_AUTO_TEST_SUITE_END()
