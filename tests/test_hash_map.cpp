
#include <boost/test/unit_test.hpp>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include "wrapped_hash_map.h"

using namespace diskhash;

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

static std::string random_key()
{
	std::string k;

	for(size_t i = 0, n = rand() % 12 + 4; i < n; i++)
	{
		k += 'a' + rand() % ('z' - 'a');
	}

	return k;
}

BOOST_AUTO_TEST_SUITE(hash_map_suite)

BOOST_AUTO_TEST_CASE(validity)
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

	unlink("testcat");
	unlink("testdat");
}

BOOST_AUTO_TEST_CASE(remove)
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

	unlink("test_rmcat");
	unlink("test_rmdat");
}

BOOST_AUTO_TEST_CASE(iterate_empty)
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");
	auto it = map1.begin();
	auto end = map1.end();
	BOOST_CHECK(it == end);
	map1.close();
	unlink("test_itercat");
	unlink("test_iterdat");
}

BOOST_AUTO_TEST_CASE(iterate)
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
	unlink("test_itercat");
	unlink("test_iterdat");
}

BOOST_AUTO_TEST_SUITE_END()

// Performance test in separate suite, disabled by default
BOOST_AUTO_TEST_SUITE(hash_map_perf, * boost::unit_test::disabled())

static void perf_inner(size_t records_count)
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
	}

	unlink("testcat");
	unlink("testdat");

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

BOOST_AUTO_TEST_CASE(performance)
{
	srand(time(0));

	for(size_t n = 4096; n <= 4 * 1024*1024; n <<= 1)
	{
		perf_inner(n);
	}
}

BOOST_AUTO_TEST_SUITE_END()
