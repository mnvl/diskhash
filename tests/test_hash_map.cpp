
#include <boost/test/unit_test.hpp>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>

#include "wrapped_hash_map.h"

using namespace diskhash;

namespace {

void cleanup_hash_map_files(const char *base)
{
	std::string cat = std::string(base) + "cat";
	std::string dat = std::string(base) + "dat";
	unlink(cat.c_str());
	unlink(dat.c_str());
}

struct validity_fixture {
	validity_fixture() { cleanup_hash_map_files("test"); }
	~validity_fixture() { cleanup_hash_map_files("test"); }
};

struct remove_fixture {
	remove_fixture() { cleanup_hash_map_files("test_rm"); }
	~remove_fixture() { cleanup_hash_map_files("test_rm"); }
};

struct iterate_fixture {
	iterate_fixture() { cleanup_hash_map_files("test_iter"); }
	~iterate_fixture() { cleanup_hash_map_files("test_iter"); }
};

struct perf_fixture {
	perf_fixture() { cleanup_hash_map_files("test"); }
	~perf_fixture() { cleanup_hash_map_files("test"); }
};

struct hash
{
	unsigned operator ()(const std::string &k)
	{
		unsigned result = 0;

		for(size_t i = 0; i < k.size(); i++)
		{
			result = -13 * result  + 7 * (k[i] - 'a') + 111;
		}

		return result;
	}
};

std::string random_key()
{
	std::string k;

	for(size_t i = 0, n = rand() % 12 + 4; i < n; i++)
	{
		k += 'a' + rand() % ('z' - 'a');
	}

	return k;
}

}

BOOST_AUTO_TEST_SUITE(hash_map_suite)

BOOST_FIXTURE_TEST_CASE(validity, validity_fixture)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test");
	std::map<std::string, unsigned> map2;

	for(int i = 0; i < 0x4000; i++)
	{
		std::string k = random_key();

		map1[k] = i;
		map2[k] = i;

		if((i & (i - 1)) == 0)
		{
			for(std::map<std::string, unsigned>::const_iterator it2 = map2.begin(); it2 != map2.end(); it2++)
			{
				BOOST_CHECK_EQUAL(map1[it2->first], it2->second);
			}
		}
	}

	for(std::map<std::string, unsigned>::const_iterator it2 = map2.begin(); it2 != map2.end(); it2++)
	{
		BOOST_CHECK_EQUAL(map1[it2->first], it2->second);
	}

	map1.close();
}

BOOST_FIXTURE_TEST_CASE(remove, remove_fixture)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_rm");
	std::map<std::string, unsigned> map2;

	for(int i = 0; i < 0x4000; i++)
	{
		std::string k = random_key();

		map1[k] = i;
		map2[k] = i;
	}

	BOOST_CHECK(!map1.remove("zzzzzzzzz_nonexistent"));

	std::map<std::string, unsigned> to_remove;
	bool keep = false;
	for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
	{
		if(!keep)
		{
			to_remove.insert(*it);
		}
		keep = !keep;
	}

	for(std::map<std::string, unsigned>::const_iterator it = to_remove.begin(); it != to_remove.end(); it++)
	{
		BOOST_CHECK(map1.remove(it->first));
		map2.erase(it->first);
	}

	for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
	{
		BOOST_CHECK_EQUAL(map1[it->first], it->second);
	}

	for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
	{
		BOOST_CHECK(map1.remove(it->first));
	}

	for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
	{
		BOOST_CHECK(!map1.remove(it->first));
	}

	map1.close();
}

BOOST_FIXTURE_TEST_CASE(iterate_empty, iterate_fixture)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");
	auto it = map1.begin();
	auto end = map1.end();
	BOOST_CHECK(it == end);
	map1.close();
}

BOOST_FIXTURE_TEST_CASE(iterate, iterate_fixture)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");
	std::map<std::string, unsigned> map2;

	srand(42);

	for(int i = 0; i < 0x1000; i++)
	{
		std::string k = random_key();
		map1[k] = i;
		map2[k] = i;
	}

	std::map<std::string, unsigned> collected;
	for(auto it = map1.begin(); it != map1.end(); ++it)
	{
		auto [key, value] = *it;
		std::string k(key.data(), key.size());
		unsigned v;
		BOOST_REQUIRE_EQUAL(value.size(), sizeof(unsigned));
		std::copy(value.data(), value.data() + sizeof(unsigned),
			reinterpret_cast<char *>(&v));
		collected[k] = v;
	}

	BOOST_CHECK_EQUAL(collected.size(), map2.size());
	for(auto const &[k, v] : map2)
	{
		auto it = collected.find(k);
		BOOST_REQUIRE(it != collected.end());
		BOOST_CHECK_EQUAL(it->second, v);
	}

	map1.close();
}

BOOST_FIXTURE_TEST_CASE(iterate_with_insert, iterate_fixture)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");

	srand(123);

	// insert initial records
	for(int i = 0; i < 100; i++)
	{
		map1[random_key()] = i;
	}

	// iterate while inserting new records
	// the iterator may skip or repeat records, but must not crash
	size_t iterations = 0;
	const size_t max_iterations = 10000; // safety limit
	for(auto it = map1.begin(); it != map1.end() && iterations < max_iterations; ++it)
	{
		iterations++;
		// insert a new record every 10 iterations
		if(iterations % 10 == 0)
		{
			map1[random_key()] = static_cast<unsigned>(iterations);
		}
	}

	BOOST_CHECK(iterations < max_iterations); // should terminate
	BOOST_CHECK(iterations >= 100); // should see at least initial records

	map1.close();
}

BOOST_FIXTURE_TEST_CASE(iterate_with_remove, iterate_fixture)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");
	std::vector<std::string> keys;

	srand(456);

	// insert records and remember keys
	for(int i = 0; i < 200; i++)
	{
		std::string k = random_key();
		map1[k] = i;
		keys.push_back(k);
	}

	// iterate while removing records
	// the iterator may skip or repeat records, but must not crash
	size_t iterations = 0;
	size_t removals = 0;
	const size_t max_iterations = 10000; // safety limit
	for(auto it = map1.begin(); it != map1.end() && iterations < max_iterations; ++it)
	{
		iterations++;
		// remove a record every 5 iterations
		if(iterations % 5 == 0 && removals < keys.size())
		{
			map1.remove(keys[removals]);
			removals++;
		}
	}

	BOOST_CHECK(iterations < max_iterations); // should terminate

	map1.close();
}

BOOST_FIXTURE_TEST_CASE(iterate_with_mixed_mutations, iterate_fixture)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");
	std::vector<std::string> keys;

	srand(789);

	// insert initial records
	for(int i = 0; i < 500; i++)
	{
		std::string k = random_key();
		map1[k] = i;
		keys.push_back(k);
	}

	// iterate while both inserting and removing
	size_t iterations = 0;
	size_t removals = 0;
	const size_t max_iterations = 20000; // safety limit
	for(auto it = map1.begin(); it != map1.end() && iterations < max_iterations; ++it)
	{
		iterations++;

		// insert new records
		if(iterations % 7 == 0)
		{
			std::string k = random_key();
			map1[k] = static_cast<unsigned>(iterations);
			keys.push_back(k);
		}

		// remove existing records
		if(iterations % 11 == 0 && removals < keys.size() / 2)
		{
			map1.remove(keys[removals]);
			removals++;
		}
	}

	BOOST_CHECK(iterations < max_iterations); // should terminate

	map1.close();
}

BOOST_AUTO_TEST_SUITE_END()

// Performance test in separate suite, disabled by default
BOOST_AUTO_TEST_SUITE(hash_map_perf, * boost::unit_test::disabled())

namespace {

void perf_inner(size_t records_count)
{
	size_t data_size = 0;

	#if defined(WIN32)
	LARGE_INTEGER c1;
	QueryPerformanceCounter(&c1);
	#else
	struct timespec c1;
	clock_gettime(CLOCK_REALTIME, &c1);
	#endif

	size_t bytes_allocated;

	{
		wrapped_hash_map<std::string, unsigned, hash> map1("test");

		for(unsigned i = 0; i < records_count; i++)
		{
			std::string str = random_key();
			map1[str] = i;

			data_size += str.size();
			data_size += sizeof(i);
		}

		bytes_allocated = map1.bytes_allocated() - records_count * sizeof(size_t);
		map1.close();
	}

	cleanup_hash_map_files("test");

	#if defined(WIN32)
	LARGE_INTEGER c2;
	QueryPerformanceCounter(&c2);
	#else
	struct timespec c2;
	clock_gettime(CLOCK_REALTIME, &c2);
	#endif

	{
		std::map<std::string, unsigned> map2;

		for(unsigned i = 0; i < records_count; i++)
		{
			map2[random_key()] = i;
		}
	}

	#if defined(WIN32)
	LARGE_INTEGER c3;
	QueryPerformanceCounter(&c3);
	#else
	struct timespec c3;
	clock_gettime(CLOCK_REALTIME, &c3);
	#endif

	#if defined(WIN32)
	LARGE_INTEGER ticks_per_sec;
	QueryPerformanceFrequency((LARGE_INTEGER *) &ticks_per_sec);

	double delta1 = double(c2.QuadPart - c1.QuadPart) / double(ticks_per_sec.QuadPart),
		delta2 = (c3.QuadPart - c2.QuadPart) / double(ticks_per_sec.QuadPart);
	#else
	double delta1 = double(c2.tv_sec - c1.tv_sec) + double(c2.tv_nsec - c1.tv_nsec) / (1000 * 1000 * 1000),
		delta2 = double(c3.tv_sec - c2.tv_sec) + double(c3.tv_nsec - c2.tv_nsec + 1) / (1000 * 1000 * 1000);
	#endif

	printf("%.4f\t%.4f\t%.4f\t%lu\t%lu\t%lu\t%.2f\n", delta1, delta2, delta1 / delta2,
		records_count, data_size, bytes_allocated, double(bytes_allocated) / data_size);
}

}

BOOST_FIXTURE_TEST_CASE(performance, perf_fixture)
{
	srand(time(0));

	for(size_t n = 4096; n <= 4 * 1024*1024; n <<= 1)
	{
		perf_inner(n);
	}
}

BOOST_AUTO_TEST_SUITE_END()
